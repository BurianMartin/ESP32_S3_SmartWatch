#include "LilyGoWatch.h"

bool LilyGoWatch::Init(Stream *stream)
{
    pinMode(BOARD_TOUCH_INT, INPUT);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    if (stream)
        ScanDevicePort(&Wire, stream);

    Wire1.begin(BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    if (stream)
        ScanDevicePort(&Wire1, stream);

    Serial.println("Initializating Power Management Unit ... ");
    if (!PowerComponents())
    {
        Serial.println("PMU Initialization failed ! ");
        return false;
    }
    else
        Serial.println("PMU Init Success!");

    ledcSetup(LEDC_BACKLIGHT_CHANNEL, LEDC_BACKLIGHT_FREQ, LEDC_BACKLIGHT_BIT_WIDTH);
    ledcAttachPin(BOARD_TFT_BL, LEDC_BACKLIGHT_CHANNEL);

    Serial.println("Init TFT");
    TFT_eSPI::init();
    setRotation(2);
    setTextDatum(MC_DATUM);
    setTextFont(2);
    Serial.println("Init TFT success");

    if (!LittleFS.begin())
        Serial.println("LittleFS Mount Failed");
    else
        LoadSettingsFromJson(true);

    /*
    Serial.println("Init FFat");
    if (!FFat.begin())
    {
        SetDisplayBrightnessPercent(50);
        drawString("Format FFat...", 120, 120);
        FFat.format();
    }
    */

    fillScreen(TFT_BLACK);
    drawString("BUR0266 - Bc. Project", 120, 120);
    SetDisplayBrightnessPercent(50);

    Serial.println("Init Touch");

    bool Result = TouchDrvFT6X36::begin(Wire1, FT6X36_SLAVE_ADDRESS, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    if (!Result)
        Serial.println("Failed to find FT6X36 - check your wiring!");
    else
    {
        Serial.println("Initializing FT6X36 succeeded");
        interruptTrigger(); // enable Interrupt
    }

#ifdef ACCELEROMETER
    SetUpAccelerometer();
#endif

    Serial.println("Init PCF8563 RTC");
    Result = SensorPCF8563::init(Wire);
    if (!Result)
        Serial.println("Failed to find PCF8563 - check your wiring!");
    else
    {
        Serial.println("Initializing PCF8563 succeeded");
        disableCLK();  // Disable clock output ， Conserve Backup Battery Current Consumption
        hwClockRead(); // Synchronize RTC clock to system clock
    }

    Serial.println("Init DRV2605");
    Result = SensorDRV2605::init(Wire);
    if (!Result)
        Serial.println("Failed to find DRV2605 - check your wiring!");
    else
    {
        Serial.println("Initializing DRV2605 succeeded");
        SensorDRV2605::selectLibrary(1);
        SensorDRV2605::setMode(DRV2605_MODE_INTTRIG);
        SensorDRV2605::useERM();
        SensorDRV2605::setWaveform(0, 15); // play effect
        SensorDRV2605::setWaveform(1, 0);  // end waveform
        SensorDRV2605::run();
    }

    // Serial.println("Init Radio SPI Bus");
    // radioBus.begin(BOARD_RADIO_SCK, BOARD_RADIO_MISO, BOARD_RADIO_MOSI);

    Serial.println("Init Persistent Storage");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());

    BeginTemp();

    SetUpAudio();

    InitMicrophone();

    Serial.print("Init Wifi\n");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    WiFi.onEvent(wifi_.Event);
    Serial.print("Init Wifi Success\n");

    delay(INTRO_DELAY);

    return true;
}

bool LilyGoWatch::FastInit(Stream *s)
{
    stream = s;

    // PMU first — controls all power rails
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    if (!PowerComponents())
        return false;

    // LEDC state is lost after deep sleep — must reconfigure
    ledcSetup(LEDC_BACKLIGHT_CHANNEL, LEDC_BACKLIGHT_FREQ, LEDC_BACKLIGHT_BIT_WIDTH);
    ledcAttachPin(BOARD_TFT_BL, LEDC_BACKLIGHT_CHANNEL);

    // TFT controller loses state during deep sleep
    TFT_eSPI::init();
    setRotation(2);
    setTextDatum(MC_DATUM);
    setTextFont(2);

    // Load settings before first draw
    if (LittleFS.begin())
        LoadSettingsFromJson(false);

    // Touch controller
    Wire1.begin(BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    if (TouchDrvFT6X36::begin(Wire1, FT6X36_SLAVE_ADDRESS, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL))
        interruptTrigger();

    // RTC — needed for correct time display immediately
    if (SensorPCF8563::init(Wire))
        hwClockRead();

#ifdef ACCELEROMETER
    // PowerComponents() briefly disables/re-enables BLDO1, which power-cycles
    // the BMA423 and resets all feature config. Full re-init is required.
    SetUpAccelerometer();
#endif

    return true;
}

void LilyGoWatch::RestoreFromDeepSleep(uint8_t brightness, int16_t menu)
{
    DisplayBrightness = brightness;
    current_menu      = menu;
    RedrawScreen      = true;
    SetDisplayBrightnessPercent(brightness);
}

void LilyGoWatch::BeginTemp()
{
    temp_sensor_config_t temp_sensor = {
        .dac_offset = TSENS_DAC_L2,
        .clk_div = 6,
    };
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
}


bool LilyGoWatch::PowerComponents()
{
    bool Result = XPowersAXP2101::init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL);
    if (!Result)
        return false;

    // Set the minimum common working voltage of the PMU VBUS input,
    // below this value will turn off the PMU
    setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);

    // Set the maximum current of the PMU VBUS input,
    // higher than this value will turn off the PMU
    setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_900MA);

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    setSysPowerDownVoltage(2600);

    // ! ESP32S3 VDD, Don't change
    // setDC1Voltage(3300);

    //! RTC VBAT , Don't change
    setALDO1Voltage(3300);

    //! TFT BACKLIGHT VDD , Don't change
    setALDO2Voltage(3300);

    //! Screen touch VDD , Don't change
    setALDO3Voltage(3300);

    //! Radio VDD , Don't change
    setALDO4Voltage(3300);

    //! DRV2605 enable
    setBLDO2Voltage(3300);

    //! GPS Power
    // setDC3Voltage(3300);
    // enableDC3();

    //! No use
    disableDC2();
    // disableDC3();
    disableDC4();
    disableDC5();
    disableBLDO1();
    disableCPUSLDO();
    disableDLDO1();
    disableDLDO2();
    disableALDO4();

    enableALDO1(); //! RTC VBAT
    enableALDO2(); //! TFT BACKLIGHT   VDD
    enableALDO3(); //! Screen touch VDD
    // enableALDO4(); //! Radio VDD
    enableBLDO2(); //! drv2605 enable

#ifdef ACCELEROMETER
    setBLDO1Voltage(3300);
    enableBLDO1();
#endif
    if (stream)
    {
        stream->println("DCDC=======================================================================");
        stream->printf("DC1  : %s   Voltage:%u mV \n", isEnableDC1() ? "+" : "-", getDC1Voltage());
        stream->printf("DC2  : %s   Voltage:%u mV \n", isEnableDC2() ? "+" : "-", getDC2Voltage());
        stream->printf("DC3  : %s   Voltage:%u mV \n", isEnableDC3() ? "+" : "-", getDC3Voltage());
        stream->printf("DC4  : %s   Voltage:%u mV \n", isEnableDC4() ? "+" : "-", getDC4Voltage());
        stream->printf("DC5  : %s   Voltage:%u mV \n", isEnableDC5() ? "+" : "-", getDC5Voltage());
        stream->println("ALDO=======================================================================");
        stream->printf("ALDO1 : %s   Voltage:%u mV\t(RTC)\n", isEnableALDO1() ? "+" : "-", getALDO1Voltage());
        stream->printf("ALDO2 : %s   Voltage:%u mV\t(Display)\n", isEnableALDO2() ? "+" : "-", getALDO2Voltage());
        stream->printf("ALDO3 : %s   Voltage:%u mV\t(Screen Touch)\n", isEnableALDO3() ? "+" : "-", getALDO3Voltage());
        stream->printf("ALDO4 : %s   Voltage:%u mV\t(Radio)\n", isEnableALDO4() ? "+" : "-", getALDO4Voltage());
        stream->println("BLDO=======================================================================");
        stream->printf("BLDO1 : %s   Voltage:%u mV\t(Accelerometer)\n", isEnableBLDO1() ? "+" : "-", getBLDO1Voltage());
        stream->printf("BLDO2 : %s   Voltage:%u mV\t(DRV2605)\n", isEnableBLDO2() ? "+" : "-", getBLDO2Voltage());
        stream->println("CPUSLDO====================================================================");
        stream->printf("CPUSLDO: %s Voltage:%u mV\n", isEnableCPUSLDO() ? "+" : "-", getCPUSLDOVoltage());
        stream->println("DLDO=======================================================================");
        stream->printf("DLDO1: %s   Voltage:%u mV\n", isEnableDLDO1() ? "+" : "-", getDLDO1Voltage());
        stream->printf("DLDO2: %s   Voltage:%u mV\n", isEnableDLDO2() ? "+" : "-", getDLDO2Voltage());
        stream->println("===========================================================================");
    }

    // Set the time of pressing the button to turn off
    setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);

    // Set the button power-on press time
    setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    disableTSPinMeasure();

    // Enable internal ADC detection
    enableBattDetection();
    enableVbusVoltageMeasure();
    enableBattVoltageMeasure();
    enableSystemVoltageMeasure();

    // t-watch no chg led
    setChargingLedMode(XPOWERS_CHG_LED_OFF);

    disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

    // Enable the required interrupt function
    enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |    // BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // CHARGE
    );

    // Clear all interrupt flags
    XPowersAXP2101::clearIrqStatus();

    // Set the precharge charging current
    setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_300MA);
    // Set stop charging termination current
    setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);

    // Set charge cut-off voltage
    setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V35);

    // Set RTC Battery voltage to 3.3V
    setButtonBatteryChargeVoltage(3300);

    enableButtonBatteryCharge();

    return true;
}

void LilyGoWatch::SetUpAccelerometer()
{
    Serial.println("Init BMA423");

    pinMode(BOARD_BMA423_INT1, INPUT);
    attachInterrupt(BOARD_BMA423_INT1, Watch.SetWakeupFlag, RISING);

    sensor.reset();
    bool Result = SensorBMA423::begin(Wire, BMA_ADDR, BOARD_I2C_SDA, BOARD_I2C_SCL);

    if (!Result)
    {
        Serial.println("Failed to find BMA423 - check your wiring!");
        return;
    }

    Serial.println("Initializing BMA423 succeeded");
    setReampAxes(REMAP_BOTTOM_LAYER_TOP_RIGHT_CORNER);

    sensor.configAccelerometer(RANGE_2G);

    // Enable acceleration sensor
    sensor.enableAccelerometer();

    // Enable sensor features
    sensor.enableFeature(SensorBMA423::FEATURE_ANY_MOTION |
                             SensorBMA423::FEATURE_ACTIVITY |
                             SensorBMA423::FEATURE_TILT |
                             SensorBMA423::FEATURE_WAKEUP,
                         true);

    // Only tilt and wakeup drive INT1 — any-motion and activity would cause
    // spurious wakeups from pocket movement and are not routed to the pin.
    sensor.enableTiltIRQ();
    sensor.enableWakeupIRQ();
    sensor.configInterrupt();
}
