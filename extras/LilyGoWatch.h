#pragma once

#include "LittleFS.h"
#include <ArduinoJson.h>

#include <Wire.h>
#include <lvgl.h>
#include <WiFi.h>
#include <vector>
#include <IRsend.h>
#include <Ticker.h>
#include <Arduino.h>
// #include <RadioLib.h>
#include <TFT_eSPI.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <WebServer.h>
#include <XPowersLib.h>
#include <driver/i2s.h>
#include <SensorBMA423.hpp>
#include <SensorDRV2605.hpp>
#include <SensorPCF8563.hpp>
#include <TouchDrvFT6X36.hpp>
#include <driver/temp_sensor.h>

#include <esp_vad.h>
#include <AudioFileSourcePROGMEM.h>

#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// extern SPIClass radioBus;

#include "LilyGoLib_Warning.h"
#include "Utilities.h"

#define GPSSerial Serial1

struct Touch
{
    int16_t startX = -1, startY = -1, furthestXfromStart = -1, furthestYfromStart = -1;
    int16_t x = -1, y = -1;
    bool touchInterruptTriggered = false, Redrawface = false;
    unsigned long lastTouchEvent = 0,
                  lastRightSwipe = 0,
                  lastLeftSwipe = 0,
                  LastUpSwipe = 0,
                  lastDownSwipe = 0;
    uint8_t Swipe = 0;
};
struct WifiResult
{
    const std::string ssid;
    int32_t signal_strength;
    bool encrypted;

    WifiResult(const std::string SSID, int32_t sigStrength, bool encrypt) : ssid(SSID), signal_strength(sigStrength), encrypted(encrypt) {};
};
struct GeneralData
{
    EventGroupHandle_t wifi_events;
    Ticker scan_ticker;
    SemaphoreHandle_t wifi_mutex;
    std::vector<WifiResult> scanResults;
    WiFiMode_t active_wifi_mode = WIFI_MODE_STA;
    WiFiMode_t wifi_mode = WIFI_MODE_NULL;
    char ip_address[16];
    uint16_t visible_networks = 0;
    bool website_on = false;
    bool handle_website_events = false;
    std::vector<std::pair<std::string, std::string>> preferred_WiFis;
};

class JsonStorage
{
public:
    JsonStorage(const char *filename, size_t capacity = 2048);
    bool load(bool printErrors = true);
    bool save(bool pretty = true);

    String getString(const char *key, const String &defaultValue = "");
    std::string getStdString(const char *key, const std::string &defaultValue = "");
    int getInt(const char *key, int defaultValue = 0);
    bool getBool(const char *key, bool defaultValue = false);

    void setStdString(const char *key, const std::string &value);
    void setString(const char *key, const String &value);
    void setInt(const char *key, int value);
    void setBool(const char *key, bool value);

    JsonArray getArray(const char *key);
    void setArray(const char *key, const JsonArray &array);

private:
    const char *filename;
    size_t capacity;
    DynamicJsonDocument doc;
};

class WiFiDriver
{
public:
    WiFiDriver(GeneralData &data);
    void Start();
    void Stop();
    void AddPreferredNetwork(const std::string &ssid, const std::string &pass);
    void RemovePreferredNetwork(const std::string &ssid);

    static void Event(WiFiEvent_t event, WiFiEventInfo_t info);

    void SetApCreds(std::string ssid, std::string pass);
    std::string GetApSSID() const;
    std::string GetApPASS() const;

private:
    std::string ap_ssid;
    std::string ap_password;
    GeneralData &data;
    void StartAccessPoint();
    static void AccessPointTask(void *param);
    static void ScanTask(void *param);
};

class Power
{
public:
    Power();

    void HandleSleepActions(uint8_t setBrightness, bool enterLightSleep);
    void WakeUpFromSleep(uint8_t setBrightness);
    void GoToSleep(bool enterLightSleep);
    void SetAlarm(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void SetWakeupFlag();
    bool ShouldSleep();

    void SetSleepTimeout(uint32_t timeout);
    void ResetIdleTime();
    void IncrementIdleTime();
    bool IsAsleep() const;

    SemaphoreHandle_t Power_Acces_Mutex;
    bool Asleep = false, ShouldWakeUp = false;
    uint16_t idle_time = 0;
    uint8_t double_touch = 0;
    int sleep_timeout;
};

struct MainWatchfaceElements
{
    lv_obj_t *dot;
    lv_obj_t *time_label;
    lv_obj_t *date_label;
    lv_obj_t *buttons[3];
    lv_obj_t *site_button;
    lv_obj_t *battery_bar;
    lv_obj_t *battery_label;
    lv_obj_t *site_status_label;
    lv_obj_t *wifi_conn_status_label;
    bool Bluetooth = false;
};

struct WifiWatchfaceElements
{
    lv_obj_t *container;
    lv_obj_t *wifi_label;
    lv_obj_t *ip_label;
    lv_obj_t *wifi_restart_button;
    lv_obj_t *mode_btn;
    lv_obj_t *network_list;
    lv_style_t list_style;
    lv_style_t list_btn_style;

    const char *WIFI_MODES_STR[4] = {"NULL",
                                     "STA",
                                     "AP",
                                     "AP_STA"};
};

struct AlarmWatchfaceElements
{
    lv_obj_t *main_cont;
    lv_obj_t *selector;
    lv_obj_t *set_alarm_btn;
    lv_obj_t *set_alarm_label;
    lv_obj_t *row_cont[3];
};

struct AudioPlayRec
{
    SemaphoreHandle_t Audio_Acces_Mutex;

    AudioOutputI2S *out;
    AudioGeneratorWAV *wav;
    AudioFileSourcePROGMEM *file;
    AudioFileSourceLittleFS *file_fs;
};

struct AudioWatchfaceComponents
{
    lv_obj_t *play_btn;
    lv_obj_t *container;
    lv_obj_t *record_btn;
    lv_obj_t *title_label;
};

struct IR_WatchfaceComponents
{
    lv_obj_t *container;
    lv_obj_t *title_label;
    lv_obj_t *btn1;
    lv_obj_t *btn2;
    lv_obj_t *btn3;
    lv_obj_t *btn4;
};

struct JsonSettings
{
    SemaphoreHandle_t Json_Acces_Mutex;

    GuiColor gui_pref_color;

    std::string time_format;
    std::string date_format;
    std::string date_language;
};

class LilyGoWatch : public TFT_eSPI,
                    public XPowersAXP2101,
                    public SensorDRV2605,
                    public SensorPCF8563,
                    public SensorBMA423,
                    public TouchDrvFT6X36,
                    public Touch,
                    public Ticker,
                    public IRsend
{
private:
    Stream *stream;

    TFT_eSprite sprite = TFT_eSprite(this);

    SensorBMA423 sensor;

    WiFiDriver wifi_;

    uint8_t DisplayBrightness = 50;

    bool RedrawScreen = true, SwipeEventsDisabled = false, Clear = false;

    Ticker WifiScanDelay, AlarmTicker, WebsiteUpdateTicker;

    Power PowerManage;
    GeneralData Globals;
    JsonSettings json_settings;
    IR_WatchfaceComponents IRFace;
    AudioWatchfaceComponents AudioGraphics;
    AudioPlayRec AudioPlayback;
    MainWatchfaceElements MainElements;
    WifiWatchfaceElements WifiElements;
    AlarmWatchfaceElements AlarmElements;

    int16_t current_menu = MAIN_FACE;

    WebServer Website;

    void HandleSleepActions();

    void UpdateScreen();

    void DrawMainFace();
    void DrawWifiFace();
    void DrawAudioFace();
    void DrawIRWatchface();
    void DrawAlarmWatchface();

    void ShowNormalState();
    void ShowPlayingState();
    void ShowRecordingState();

    void
    PDM_Record(const char *song_name, uint32_t duration);
    bool CreateFileWAV(const char *song_name, uint32_t duration, uint16_t num_channels, const uint32_t sampling_rate, uint16_t bits_per_sample);
    bool ReadMicrophone(void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait = portMAX_DELAY);

    void LoadSettingsFromJson(bool printData = false);

    void SaveSettingsToJson();

    bool InitMicrophone();

    void SetUpAccelerometer();

    void SetUpAudio();

    void InitWifi();

    bool ShouldSleep();

    // static void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

public:
    LilyGoWatch() : Website(80), IRsend(BOARD_IR_PIN), wifi_(Globals)
    {
        Globals.wifi_events = xEventGroupCreate();

        sprite.createSprite(240, 240);
        AudioPlayback.Audio_Acces_Mutex = xSemaphoreCreateBinary();
        PowerManage.Power_Acces_Mutex = xSemaphoreCreateBinary();
        Globals.wifi_mutex = xSemaphoreCreateBinary();
        json_settings.Json_Acces_Mutex = xSemaphoreCreateBinary();

        configASSERT(AudioPlayback.Audio_Acces_Mutex != NULL);
        configASSERT(PowerManage.Power_Acces_Mutex != NULL);
        configASSERT(Globals.wifi_mutex != NULL);
        configASSERT(json_settings.Json_Acces_Mutex != NULL);

        xSemaphoreGive(AudioPlayback.Audio_Acces_Mutex);
        xSemaphoreGive(PowerManage.Power_Acces_Mutex);
        xSemaphoreGive(Globals.wifi_mutex);
        xSemaphoreGive(json_settings.Json_Acces_Mutex);
    }
    ~LilyGoWatch() {}

    bool PowerComponents();
    bool Init(Stream *stream);

    WiFiDriver *getWifiDriverRef();

    void Update();
    void WakeUp();
    void NextMenu();
    void PrevMenu();
    void StopWiFi();
    void GoToSleep();
    void BeginTemp();
    void StartWiFi();

    void Add_Prefered_WiFi(std::string name, std::string pass);
    void Remove_Prefered_WiFi(std::string name);

    static void scroll_event_cb(lv_event_t *e);
    static void scroll_end_event_h(lv_event_t *e);
    static void scroll_end_event_ms(lv_event_t *e);
    static void WiFiEventHandler(WiFiEvent_t event);
    static void StartAccessPointTask(void *parameter);

    void Print(const char *message);
    void PrintLn(const char *message);
    void ScanDevicePort(TwoWire *port, Stream *stream);
    void SetDisplayBrightnessPercent(uint8_t value = -1);
    void SetAlarm(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void AddNetworkToList(const char *ssid, int signal_strength, bool is_secured);

    const char *GetTimeString();
    const char *GetDateString();

    static void AlarmHandler();
    static void SetWakeupFlag();

    static void InterruptTestFn();

    static void UpdateWatchScreen();
    static void GeneralTouchHandler();
    static void SwipeHandler(lv_event_t *e);
    static void ButtonHandler(lv_event_t *e);
    static void WaitWiFiScanFinish(void *parameter);

    static void Record(void *parameter);
    static void PlayRecordingFromStorage(void *parameter);

    static void PlayIrSignal(void *parameter);

    static void HandleWebsite(void *parameter);

    static void SetWebsiteHandleFlag();

    void HandleRoot();
    void SetupRoutes();
    void HandleWifiGet();
    void HandleDateSet();
    void HandleColorSet();
    void HandleBrightness();
    void HandleSleepTimeoutSet();
    void HandleWebsiteStop();
    void HandleNetworkListGet();
    void HandlePreferredListGet();
    void HandleWifiSetMode();
    void HandleAddPreferredNetwork();
    void HandleRemovePreferredNetwork();
    void SetApCredentials();

    bool StartServer();
    void StopServer();
    bool IsRunning() const { return Globals.website_on; }
    String GetIP() const { return WiFi.localIP().toString(); }

    uint32_t ReadRegister(int reg);
};

extern LilyGoWatch Watch;
