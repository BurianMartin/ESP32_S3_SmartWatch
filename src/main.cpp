#include <LV_Helper.h>
#include "LilyGoWatch.h"
#include "RtcState.h"

// RTC memory definitions — survive deep sleep, zero-initialised on cold boot
RTC_DATA_ATTR uint8_t rtc_display_brightness = 50;
RTC_DATA_ATTR int16_t rtc_current_menu        = MAIN_FACE;
RTC_DATA_ATTR bool    rtc_deep_sleep_wake     = false;

Ticker WatchfaceUpdateTicker;
Ticker WebsiteHandleTicker;

void setup()
{
    setCpuFrequencyMhz(80);
    Serial.begin(115200);

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    bool is_deep_sleep_wake = rtc_deep_sleep_wake &&
                              (cause == ESP_SLEEP_WAKEUP_EXT0 ||
                               cause == ESP_SLEEP_WAKEUP_EXT1);

    if (is_deep_sleep_wake)
    {
        rtc_deep_sleep_wake = false;

        bool ok = false;
        for (int i = 0; i < 3 && !ok; ++i)
            ok = Watch.FastInit(&Serial);
        if (!ok)
            esp_restart();

        Watch.RestoreFromDeepSleep(rtc_display_brightness, rtc_current_menu);
    }
    else
    {
        bool ok = false;
        for (int i = 0; i < 3 && !ok; ++i)
            ok = Watch.Init(&Serial);
        while (!ok)
            ;
    }

    Watch.IRsend::begin();

    pinMode(BOARD_PMU_INT, INPUT_PULLUP);
    attachInterrupt(BOARD_PMU_INT, Watch.SetWakeupFlag, FALLING);

    // PMU wakes on LOW (active-low), BMA423 wakes on HIGH (RISING edge).
    // EXT1 cannot mix polarities, so PMU uses EXT0 and BMA423 uses EXT1.
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BOARD_PMU_INT, 0);
#ifdef ACCELEROMETER
    esp_sleep_enable_ext1_wakeup(_BV(BOARD_BMA423_INT1), ESP_EXT1_WAKEUP_ANY_HIGH);
#endif

    beginLvglHelper(false);
    lv_obj_add_event_cb(lv_scr_act(), Watch.SwipeHandler, LV_EVENT_GESTURE, NULL);

    attachInterrupt(BOARD_TOUCH_INT, Watch.GeneralTouchHandler, RISING);

    if (!is_deep_sleep_wake)
        delay(1000);

    WatchfaceUpdateTicker.attach(1, Watch.UpdateWatchScreen);
    WebsiteHandleTicker.attach(0.5, Watch.SetWebsiteHandleFlag);

#ifdef DEBUG
    Watch.Add_Prefered_WiFi("STARNET-Burian", "pilir3453");
    Watch.StartWiFi();
#endif
}

void loop()
{
    Watch.Update();
    delay(5);
}
