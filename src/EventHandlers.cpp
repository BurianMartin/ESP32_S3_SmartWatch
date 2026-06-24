#include "LilyGoWatch.h"

void LilyGoWatch::InterruptTestFn()
{
    Serial.println("Interrupt Worked!");
    Watch.PowerManage.ShouldWakeUp = true;
    Watch.PowerManage.idle_time = 0;
}

void LilyGoWatch::UpdateWatchScreen()
{
    if (!Watch.PowerManage.Asleep)
        Watch.RedrawScreen = true, Watch.PowerManage.idle_time++;
    else
        Watch.PowerManage.idle_time = 0;
    Watch.PowerManage.double_touch = 0;
}

void LilyGoWatch::SwipeHandler(lv_event_t *e)
{
    if (Watch.SwipeEventsDisabled || Watch.PowerManage.Asleep)
        return;

    Watch.RedrawScreen = true;

    lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    switch (gesture_dir)
    {
    case LV_DIR_LEFT:
        Watch.NextMenu();
        // Serial.println("Swiped Left");
        break;
    case LV_DIR_RIGHT:
        Watch.PrevMenu();
        // Serial.println("Swiped Right");
        break;
    case LV_DIR_TOP:
        // Serial.println("Swiped Up");
        break;
    case LV_DIR_BOTTOM:
        // Serial.println("Swiped Down");
        break;
    default:
        // Serial.println("No swipe detected");
        break;
    }
}

void LilyGoWatch::ButtonHandler(lv_event_t *e)
{

    if (Watch.PowerManage.Asleep)
        return;

    lv_obj_t *button = lv_event_get_target(e);
    int button_index = (int)lv_obj_get_user_data(button);

    Watch.RedrawScreen = true;

    switch (button_index)
    {
    case 0: // Brightness Button
        Watch.DisplayBrightness += 25;
        if (Watch.DisplayBrightness > 100)
            Watch.DisplayBrightness = 25;
        Watch.SetDisplayBrightnessPercent(Watch.DisplayBrightness);
        break;

    case 1: // WiFi button
        if (Watch.Globals.wifi_mode == WIFI_MODE_NULL)
            Watch.StartWiFi();
        else
            Watch.StopServer(), Watch.StopWiFi();

        break;

    case 2: // Bluetooth button
        Watch.MainElements.Bluetooth = !Watch.MainElements.Bluetooth;
        break;

    case 3: // Site management button

        if (Watch.Globals.website_on)
        {
            Watch.StopServer();
            Watch.WebsiteUpdateTicker.detach();
        }
        else
        {
            Watch.StartServer();
            Watch.WebsiteUpdateTicker.attach(0.5, Watch.SetWebsiteHandleFlag);
        }
        break;

    case 4: // Set alarm Buttton ( currently not in use )
    {
        Watch.RedrawScreen = false;

        uint8_t min, hour, sec;

        for (uint8_t i = 0; i < 3; i++)
        {
            lv_coord_t scroll_y = lv_obj_get_scroll_y(Watch.AlarmElements.row_cont[i]);
            int32_t value = (scroll_y + 20) / 40;

            if (i == 0)
                hour = (value < 0) ? 0 : (value > 23) ? 23
                                                      : value;
            else
            {
                uint8_t *target = (i == 1 ? &min : &sec);
                *target = (value < 0) ? 0 : (value > 59) ? 59
                                                         : value;
            }
        }
        Serial.printf("Alarm set to %02d:%02d:%02d\n", hour, min, sec);

        RTC_DateTime curr_time = Watch.getDateTime();

        uint32_t current_seconds = curr_time.hour * 3600 + curr_time.minute * 60 + curr_time.second;
        uint32_t alarm_seconds_total = hour * 3600 + min * 60 + sec;

        if (alarm_seconds_total <= current_seconds)
            alarm_seconds_total += 24 * 3600;

        Watch.AlarmTicker.attach(alarm_seconds_total - current_seconds, Watch.AlarmHandler);
        break;
    }

    case 5: // Run a task to play wav file from persistent storage
        xTaskCreate(PlayRecordingFromStorage, "wavPlayer", 8 * 1024, NULL, 12, NULL);
        break;

    case 6: // Run a task to record voice
        xTaskCreate(Record, "wavRecording", 8 * 1024, NULL, 12, NULL);
        break;

    case 7:
    case 8:
    case 9:
    case 10:
        xTaskCreate(PlayIrSignal, "IR_Signal_Transmit", 4 * 1024, (void *)(button_index - 7), 12, NULL);
        break;
    case 11:
        if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
        {
            if (Watch.Globals.active_wifi_mode == WIFI_MODE_AP)
                Watch.Globals.active_wifi_mode = WIFI_MODE_STA;
            else if (Watch.Globals.active_wifi_mode == WIFI_MODE_STA)
                Watch.Globals.active_wifi_mode = WIFI_MODE_AP;

            xSemaphoreGive(Watch.Globals.wifi_mutex);
        }
        switch (Watch.Globals.active_wifi_mode)
        {
        case WIFI_MODE_AP:
            Serial.println("Changing wifi mode to: AP");
            break;

        case WIFI_MODE_STA:
            Serial.println("Changing wifi mode to: STA");
            break;

        default:
            break;
        }

        Watch.StopWiFi();
        Watch.StartWiFi();
        break;

    case 12: // Restarts the wifi, scans again and tries to connect again
        if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
        {
            Watch.Globals.wifi_mode = WIFI_MODE_NULL;
            WiFi.mode(WIFI_OFF); // reset internal state
            delay(100);

            WiFi.mode(WIFI_STA);
            delay(100);
            Watch.StopWiFi();

            WiFi.setHostname("MyESP32");
            Watch.Globals.wifi_mode = WIFI_MODE_STA;
            Watch.StartWiFi();
            xSemaphoreGive(Watch.Globals.wifi_mutex);
        }
        break;

    // --- Settings face controls ---

    case 13: // Color: cycle through GuiColor values
    {
        if (xSemaphoreTake(Watch.json_settings.Json_Acces_Mutex, portMAX_DELAY))
        {
            Watch.json_settings.gui_pref_color =
                GuiColor((Watch.json_settings.gui_pref_color + 1) % 5);
            xSemaphoreGive(Watch.json_settings.Json_Acces_Mutex);
        }
        Watch.SaveSettingsToJson();
        Watch.RedrawScreen = true;
        break;
    }

    case 14: // Time format: toggle 24h ↔ 12h
    {
        if (xSemaphoreTake(Watch.json_settings.Json_Acces_Mutex, portMAX_DELAY))
        {
            Watch.json_settings.time_format =
                (Watch.json_settings.time_format == "24h") ? "12h" : "24h";
            xSemaphoreGive(Watch.json_settings.Json_Acces_Mutex);
        }
        Watch.SaveSettingsToJson();
        Watch.RedrawScreen = true;
        break;
    }

    case 15: // Sleep timeout: decrease
    {
        static const int sleep_steps[] = {7, 10, 20, 30, 60};
        static const int step_count = 5;
        int cur = Watch.PowerManage.sleep_timeout;
        int idx = 0;
        for (int i = 0; i < step_count; i++)
            if (sleep_steps[i] == cur) { idx = i; break; }
        int next = sleep_steps[(idx > 0) ? idx - 1 : 0];
        Watch.PowerManage.SetSleepTimeout(next);
        Watch.SaveSettingsToJson();
        Watch.RedrawScreen = true;
        break;
    }

    case 16: // Sleep timeout: increase
    {
        static const int sleep_steps[] = {7, 10, 20, 30, 60};
        static const int step_count = 5;
        int cur = Watch.PowerManage.sleep_timeout;
        int idx = step_count - 1;
        for (int i = 0; i < step_count; i++)
            if (sleep_steps[i] == cur) { idx = i; break; }
        int next = sleep_steps[(idx < step_count - 1) ? idx + 1 : step_count - 1];
        Watch.PowerManage.SetSleepTimeout(next);
        Watch.SaveSettingsToJson();
        Watch.RedrawScreen = true;
        break;
    }

    case 17: // Date format: cycle DD-MM-YYYY ↔ YYYY-MM-DD
    {
        if (xSemaphoreTake(Watch.json_settings.Json_Acces_Mutex, portMAX_DELAY))
        {
            Watch.json_settings.date_format =
                (Watch.json_settings.date_format == "DD-MM-YYYY") ? "YYYY-MM-DD" : "DD-MM-YYYY";
            xSemaphoreGive(Watch.json_settings.Json_Acces_Mutex);
        }
        Watch.SaveSettingsToJson();
        Watch.RedrawScreen = true;
        break;
    }

    case 18: // Date language: toggle EN ↔ CZ
    {
        if (xSemaphoreTake(Watch.json_settings.Json_Acces_Mutex, portMAX_DELAY))
        {
            Watch.json_settings.date_language =
                (Watch.json_settings.date_language == "en") ? "cz" : "en";
            xSemaphoreGive(Watch.json_settings.Json_Acces_Mutex);
        }
        Watch.SaveSettingsToJson();
        Watch.RedrawScreen = true;
        break;
    }

    default:
        Serial.println("Button Handling missing");
        break;
    }
}

void IRAM_ATTR LilyGoWatch::GeneralTouchHandler()
{
    if (!Watch.PowerManage.Asleep)
        Watch.PowerManage.idle_time = 0;
    else
    {
        Watch.PowerManage.double_touch++;
        if (Watch.PowerManage.double_touch >= 2)
        {
            Watch.PowerManage.ShouldWakeUp = true;
            Watch.PowerManage.double_touch = 0;
        }
    }
}

void LilyGoWatch::AlarmHandler()
{
    Watch.WakeUp();
    Serial.println("Alarm triggered");
    Watch.AlarmTicker.detach();
}

void LilyGoWatch::scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    int32_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for (uint32_t i = 0; i < child_cnt; i++)
    {
        lv_obj_t *child = lv_obj_get_child(cont, i);
        if (lv_obj_get_height(child) < 10)
            continue;

        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);
        int32_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;

        int32_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        if (diff_y < 20)
            lv_obj_add_state(child, LV_STATE_USER_1);
        else
        {
            lv_obj_clear_state(child, LV_STATE_USER_1);
            lv_opa_t opa = lv_map(diff_y, 20, 100, LV_OPA_40, LV_OPA_10);
            lv_obj_set_style_text_opa(child, opa, 0);
        }
    }
}

void LilyGoWatch::scroll_end_event_h(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_coord_t y = lv_obj_get_scroll_y(cont);

    int index = (y + 20) / 40;
    if (index < 0)
        index = 0;
    else if (index > 23)
        index = 23;
    lv_obj_scroll_to_y(cont, index * 40, LV_ANIM_ON);
}

void LilyGoWatch::scroll_end_event_ms(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_coord_t y = lv_obj_get_scroll_y(cont);

    int index = (y + 20) / 40;
    if (index < 0)
        index = 0;
    else if (index > 71)
        index = 71;
    lv_obj_scroll_to_y(cont, index * 40, LV_ANIM_ON);
}
