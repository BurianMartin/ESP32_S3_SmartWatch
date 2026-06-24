#include <LV_Helper.h>
#include "LilyGoWatch.h"

Ticker WatchfaceUpdateTicker;
Ticker WebsiteHandleTicker;

void setup()
{
    setCpuFrequencyMhz(80);
    Serial.begin(115200);

    bool watch_init_success = false;

    for (int i = 0; i < 3 && !watch_init_success; ++i)
        watch_init_success = Watch.Init(&Serial);

    while (!watch_init_success)
        ;

    irsend.begin();

    pinMode(BOARD_PMU_INT, INPUT_PULLUP);
    attachInterrupt(BOARD_PMU_INT, Watch.SetWakeupFlag, FALLING);

    esp_sleep_enable_ext1_wakeup(_BV(BOARD_PMU_INT), ESP_EXT1_WAKEUP_ALL_LOW);

    beginLvglHelper(false);
    lv_obj_add_event_cb(lv_scr_act(), Watch.SwipeHandler, LV_EVENT_GESTURE, NULL);

    attachInterrupt(BOARD_TOUCH_INT, Watch.GeneralTouchHandler, RISING);

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
