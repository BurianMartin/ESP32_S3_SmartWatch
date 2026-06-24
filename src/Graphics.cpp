#include "LilyGoWatch.h"

void LilyGoWatch::Update()
{
    HandleSleepActions();

    UpdateScreen();

    if (Watch.Globals.website_on && Watch.Globals.handle_website_events && Watch.Globals.website_task_handle == NULL)
        xTaskCreate(HandleWebsite, "WebsiteHandler", 4 * 1024, NULL, 12, &Watch.Globals.website_task_handle);

    lv_timer_handler();
}

void LilyGoWatch::UpdateScreen()
{
    if (RedrawScreen)
    {
        RedrawScreen = false;
        lv_obj_clean(lv_scr_act());
        switch (current_menu)
        {
        case WIFI_FACE:
            DrawWifiFace();
            break;

        case MAIN_FACE:
            DrawMainFace();
            break;

        case ALARM_FACE:
            DrawAlarmWatchface();
            break;

        case IR_FACE:
            DrawIRWatchface();
            break;

        case SPEAKER_FACE:
            DrawAudioFace();
            break;

        default:
            break;
        }
    }
}

void LilyGoWatch::DrawMainFace()
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(0, 0, 0), LV_PART_MAIN);

    MainElements.date_label = lv_label_create(lv_scr_act());
    lv_label_set_text(MainElements.date_label, GetDateString());
    lv_obj_set_style_text_font(MainElements.date_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.date_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(MainElements.date_label, LV_ALIGN_TOP_LEFT, 10, 10);

    MainElements.battery_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(MainElements.battery_bar, 40, 10);
    lv_obj_align(MainElements.battery_bar, LV_ALIGN_TOP_RIGHT, -48, 12);
    lv_bar_set_value(MainElements.battery_bar, getBatteryPercent(), LV_ANIM_OFF);

    if (isCharging())
        lv_obj_set_style_bg_color(MainElements.battery_bar, lv_color_hex(rgbToHex(0, 200, 0)), LV_PART_INDICATOR);
    else
        lv_obj_set_style_bg_color(MainElements.battery_bar, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), LV_PART_INDICATOR);

    MainElements.battery_label = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(MainElements.battery_label, "%d%%", getBatteryPercent());
    lv_obj_set_style_text_font(MainElements.battery_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.battery_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align_to(MainElements.battery_label, MainElements.battery_bar, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

    MainElements.wifi_conn_status_label = lv_label_create(lv_scr_act());
    lv_label_set_text(MainElements.wifi_conn_status_label, WiFi.status() == WL_CONNECTED ? "C" : "");
    lv_obj_set_style_text_font(MainElements.wifi_conn_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.wifi_conn_status_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align_to(MainElements.wifi_conn_status_label, MainElements.battery_bar, LV_ALIGN_OUT_RIGHT_MID, -75, 0);

    MainElements.time_label = lv_label_create(lv_scr_act());
    lv_label_set_text(MainElements.time_label, GetTimeString());
    lv_obj_set_style_text_font(MainElements.time_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.time_label, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_align(MainElements.time_label, LV_ALIGN_RIGHT_MID, -27, -57);

    static const char *btn_labels[] = {"Bright", "WiFi", "BT"};
    for (int i = 0; i < 3; i++)
    {
        MainElements.buttons[i] = lv_btn_create(lv_scr_act());
        lv_obj_set_size(MainElements.buttons[i], 55, 55);
        lv_obj_set_style_radius(MainElements.buttons[i], 28, 0);
        lv_obj_align(MainElements.buttons[i], LV_ALIGN_TOP_LEFT, 15, 35 + (i * 70));
        lv_obj_add_event_cb(MainElements.buttons[i], ButtonHandler, LV_EVENT_CLICKED, NULL);

        lv_obj_set_user_data(MainElements.buttons[i], (void *)i);

        lv_obj_t *btn_label = lv_label_create(MainElements.buttons[i]);
        lv_label_set_text(btn_label, btn_labels[i]);
        lv_obj_center(btn_label);

        if (i == 2 && MainElements.Bluetooth || i == 1 && Globals.wifi_mode != WIFI_MODE_NULL)
        {
            lv_obj_set_style_bg_color(MainElements.buttons[i], lv_color_hex(0x000000), LV_PART_MAIN);
            lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
        else if (i == 0)
        {
            uint16_t rgb = 255 - DisplayBrightness * 2.55;
            lv_obj_set_style_bg_color(MainElements.buttons[i], lv_color_make(rgb, rgb, rgb), LV_PART_MAIN);
            if (DisplayBrightness <= 50)
                lv_obj_set_style_text_color(btn_label, lv_color_hex(0x000000), LV_PART_MAIN);
            else
                lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_bg_color(MainElements.buttons[i], lv_color_hex(0xCECECE), LV_PART_MAIN);
            lv_obj_set_style_text_color(btn_label, lv_color_hex(0x000000), LV_PART_MAIN);
        }

        lv_obj_set_style_outline_color(MainElements.buttons[i], lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), LV_PART_MAIN);
        lv_obj_set_style_outline_width(MainElements.buttons[i], 2, LV_PART_MAIN);
        lv_obj_set_style_outline_pad(MainElements.buttons[i], 0, LV_PART_MAIN);
    }

    MainElements.site_status_label = lv_label_create(lv_scr_act());
    lv_label_set_text(MainElements.site_status_label, "Site Status: ");
    lv_obj_set_style_text_font(MainElements.site_status_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.site_status_label, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_align(MainElements.site_status_label, LV_ALIGN_RIGHT_MID, -40, 0);

    MainElements.dot = lv_obj_create(lv_scr_act());
    lv_obj_set_size(MainElements.dot, 13, 13);

    int dotColor = 0xFF0000;

    if (Globals.website_on)
        dotColor = 0x00FF00;

    lv_obj_set_style_bg_color(MainElements.dot, lv_color_hex(dotColor), LV_PART_MAIN);

    lv_obj_set_style_arc_color(MainElements.dot, lv_color_hex(dotColor), LV_PART_MAIN);

    lv_obj_set_style_outline_width(MainElements.dot, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(MainElements.dot, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_border_width(MainElements.dot, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(MainElements.dot, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_shadow_width(MainElements.dot, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(MainElements.dot, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_align(MainElements.dot, LV_ALIGN_RIGHT_MID, -25, 1);

    MainElements.site_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(MainElements.site_button, 120, 45);
    lv_obj_set_style_radius(MainElements.site_button, 20, 0);
    lv_obj_align(MainElements.site_button, LV_ALIGN_RIGHT_MID, -28, 52);
    lv_obj_add_event_cb(MainElements.site_button, ButtonHandler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(MainElements.site_button);
    if (Globals.website_on)
        lv_label_set_text(btn_label, "Turn Site Off");
    else
        lv_label_set_text(btn_label, "Turn Site On");

    lv_obj_center(btn_label);
    lv_obj_set_user_data(MainElements.site_button, (void *)3);

    lv_obj_set_style_bg_color(MainElements.site_button, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_outline_color(MainElements.site_button, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), LV_PART_MAIN);
    lv_obj_set_style_outline_width(MainElements.site_button, 2, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(MainElements.site_button, 0, LV_PART_MAIN);
}

void LilyGoWatch::DrawWifiFace()
{
    WifiElements.container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(WifiElements.container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(WifiElements.container, lv_color_black(), 0);
    lv_obj_set_style_border_color(WifiElements.container, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_bg_opa(WifiElements.container, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(WifiElements.container, 10, 0);
    lv_obj_set_scroll_dir(WifiElements.container, LV_DIR_NONE);

    WifiElements.wifi_label = lv_label_create(WifiElements.container);
    lv_label_set_text(WifiElements.wifi_label, "WiFi");
    lv_obj_set_style_text_color(WifiElements.wifi_label, lv_color_white(), 0);
    lv_obj_align(WifiElements.wifi_label, LV_ALIGN_TOP_LEFT, 10, 10);

    auto set_button_style = [](lv_obj_t *btn, uint32_t color, int id)
    {
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
        lv_obj_set_style_shadow_width(btn, 8, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(color), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
        lv_obj_set_user_data(btn, (void *)(intptr_t)id);
        lv_obj_add_event_cb(btn, ButtonHandler, LV_EVENT_CLICKED, NULL);
    };

    WifiElements.wifi_restart_button = lv_btn_create(WifiElements.container);
    set_button_style(WifiElements.wifi_restart_button, 0x2ECC71, 12);
    lv_obj_align(WifiElements.wifi_restart_button, LV_ALIGN_TOP_MID, -15, 3);
    lv_obj_set_size(WifiElements.wifi_restart_button, 70, 30);

    lv_obj_t *reset_btn_label = lv_label_create(WifiElements.wifi_restart_button);
    lv_label_set_text(reset_btn_label, "WiFi RST");
    lv_obj_center(reset_btn_label);

    WifiElements.mode_btn = lv_btn_create(WifiElements.container);
    lv_obj_set_size(WifiElements.mode_btn, 60, 30);
    lv_obj_align(WifiElements.mode_btn, LV_ALIGN_TOP_RIGHT, -5, 3);
    set_button_style(WifiElements.mode_btn, 0x3498DB, 11);

    lv_obj_t *mode_label = lv_label_create(WifiElements.mode_btn);
    lv_label_set_text(mode_label, WifiElements.WIFI_MODES_STR[Globals.wifi_mode]);
    lv_obj_set_style_text_color(mode_label, lv_color_white(), 0);
    lv_obj_center(mode_label);

    lv_style_init(&WifiElements.list_style);
    lv_style_set_bg_color(&WifiElements.list_style, lv_color_black());
    lv_style_set_text_color(&WifiElements.list_style, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&WifiElements.list_style, 5);

    lv_style_init(&WifiElements.list_btn_style);
    lv_style_set_bg_opa(&WifiElements.list_btn_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&WifiElements.list_btn_style, 0);
    lv_style_set_pad_all(&WifiElements.list_btn_style, 5);

    WifiElements.network_list = lv_list_create(WifiElements.container);

    lv_obj_set_size(WifiElements.network_list, 230, 165); // Adjust size as needed
    lv_obj_align_to(WifiElements.network_list, WifiElements.wifi_label, LV_ALIGN_OUT_RIGHT_TOP, -50, 33);
    lv_obj_add_style(WifiElements.network_list, &WifiElements.list_style, 0);

    for (auto record : Globals.scanResults)
        AddNetworkToList(record.ssid.c_str(), record.signal_strength, record.encrypted);

    WifiElements.ip_label = lv_label_create(WifiElements.container);

    switch (Globals.wifi_mode)
    {
    case WIFI_MODE_STA:
        lv_label_set_text(WifiElements.ip_label, (WiFi.status() == WL_CONNECTED ? String("IP Address: " + WiFi.localIP().toString()).c_str() : "WiFi Not Connected"));
        break;
    case WIFI_MODE_AP:
        lv_label_set_text(WifiElements.ip_label, WiFi.softAPIP().toString().c_str());
        break;

    case WIFI_MODE_NULL:
        lv_label_set_text(WifiElements.ip_label, "WiFi Off");
        break;

    default:
        break;
    }

    lv_obj_set_style_text_color(WifiElements.ip_label, lv_color_white(), 0);
    lv_obj_align(WifiElements.ip_label, LV_ALIGN_BOTTOM_MID, 0, 9);
}

void LilyGoWatch::AddNetworkToList(const char *ssid, int signal_strength, bool is_secured)
{
    lv_obj_t *btn = lv_list_add_btn(WifiElements.network_list, NULL, NULL);
    lv_obj_add_style(btn, &WifiElements.list_btn_style, 0);

    lv_obj_t *cont = lv_obj_create(btn);
    lv_obj_set_size(cont, 200, 30);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    std::string ssidTruncated;
    if (strlen(ssid) > 14)
        ssidTruncated = std::string(ssid, 11) + "...";
    const char *ssidToPrint = (strlen(ssid) > 14 ? ssidTruncated.c_str() : ssid);
    lv_obj_t *ssid_label = lv_label_create(cont);
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(ssid_label, ssidToPrint);
    lv_obj_set_width(ssid_label, 115);
    lv_obj_align(ssid_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *signal_label = lv_label_create(cont);
    lv_obj_set_style_text_color(signal_label, lv_color_hex(0xFFFFFF), 0);
    if (signal_strength >= -50)
        lv_label_set_text(signal_label, LV_SYMBOL_WIFI);
    else if (signal_strength >= -70)
        lv_label_set_text(signal_label, "III");
    else if (signal_strength >= -80)
        lv_label_set_text(signal_label, "II");
    else
        lv_label_set_text(signal_label, "I");

    if (is_secured)
    {
        lv_obj_t *lock_label = lv_label_create(cont);
        lv_label_set_text(lock_label, LV_SYMBOL_CLOSE);
    }
}

void LilyGoWatch::DrawAlarmWatchface()
{
    AlarmElements.main_cont = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(AlarmElements.main_cont);
    lv_obj_set_size(AlarmElements.main_cont, 240, 240);
    lv_obj_center(AlarmElements.main_cont);
    lv_obj_set_style_bg_color(AlarmElements.main_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(AlarmElements.main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(AlarmElements.main_cont, 0, 0);
    lv_obj_set_scroll_dir(AlarmElements.main_cont, LV_DIR_NONE);

    AlarmElements.set_alarm_btn = lv_btn_create(AlarmElements.main_cont);
    lv_obj_set_size(AlarmElements.set_alarm_btn, 160, 40);
    lv_obj_set_style_bg_color(AlarmElements.set_alarm_btn, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_bg_opa(AlarmElements.set_alarm_btn, LV_OPA_COVER, 0);
    lv_obj_align(AlarmElements.set_alarm_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_user_data(AlarmElements.set_alarm_btn, (void *)4);
    lv_obj_add_event_cb(AlarmElements.set_alarm_btn, ButtonHandler, LV_EVENT_CLICKED, NULL);

    AlarmElements.set_alarm_label = lv_label_create(AlarmElements.set_alarm_btn);
    lv_label_set_text(AlarmElements.set_alarm_label, "Set Alarm");
    lv_obj_center(AlarmElements.set_alarm_label);

    static lv_obj_t *number_labels[3][60];

    for (uint8_t col = 0; col < 3; col++)
    {
        AlarmElements.row_cont[col] = lv_obj_create(AlarmElements.main_cont);
        lv_obj_remove_style_all(AlarmElements.row_cont[col]);
        lv_obj_set_size(AlarmElements.row_cont[col], 80, 180);
        lv_obj_set_pos(AlarmElements.row_cont[col], col * 80, 0);

        lv_obj_set_style_bg_opa(AlarmElements.row_cont[col], LV_OPA_0, 0);
        lv_obj_set_style_pad_all(AlarmElements.row_cont[col], 0, 0);
        lv_obj_set_scroll_dir(AlarmElements.row_cont[col], LV_DIR_VER);
        lv_obj_set_scroll_snap_y(AlarmElements.row_cont[col], LV_SCROLL_SNAP_CENTER);
        lv_obj_set_scrollbar_mode(AlarmElements.row_cont[col], LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *cont = lv_obj_create(AlarmElements.row_cont[col]);
        lv_obj_remove_style_all(cont);
        lv_obj_set_size(cont, 80, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(cont, 0, 0);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_flex_main_place(cont, LV_FLEX_ALIGN_START, 0);
        lv_obj_set_scroll_dir(cont, LV_DIR_VER);

        lv_obj_t *top_pad = lv_obj_create(cont);
        lv_obj_remove_style_all(top_pad);
        lv_obj_set_size(top_pad, 80, 70);
        lv_obj_set_style_bg_opa(top_pad, LV_OPA_0, 0);

        uint32_t max_val = (col == 0) ? 23 : 59;
        for (uint32_t i = 0; i <= max_val; i++)
        {
            lv_obj_t *label = lv_label_create(cont);
            number_labels[col][i] = label;
            lv_obj_remove_style_all(label);

            lv_obj_set_size(label, 80, 40);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_pad_all(label, 0, 0);

            lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(label, lv_color_white(), 0);

            char buf[8];
            lv_snprintf(buf, sizeof(buf), "%02d", i);
            lv_label_set_text(label, buf);
        }

        lv_obj_t *bottom_pad = lv_obj_create(cont);
        lv_obj_remove_style_all(bottom_pad);
        lv_obj_set_size(bottom_pad, 80, 70);
        lv_obj_set_style_bg_opa(bottom_pad, LV_OPA_0, 0);

        lv_obj_add_event_cb(AlarmElements.row_cont[col], [](lv_event_t *e)
                            {
            lv_obj_t* obj = lv_event_get_target(e);
            uint8_t col_index = (uint8_t)(uintptr_t)lv_obj_get_user_data(obj);
            
            lv_coord_t scroll_y = lv_obj_get_scroll_y(obj);
            int32_t centered_index = (scroll_y + 20) / 40;  
            uint32_t max_val = (col_index == 0) ? 23 : 59;
            
            for (uint32_t i = 0; i <= max_val; i++) {
                lv_obj_t* label = number_labels[col_index][i];
                if (i == centered_index) {
                    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
                    lv_obj_set_style_text_color(label, lv_color_hex(HIGHLIGHT_COLORS[Watch.json_settings.gui_pref_color]), 0);
                } else {
                    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_text_color(label, lv_color_white(), 0);
                }
            } }, LV_EVENT_SCROLL, NULL);

        lv_obj_set_user_data(AlarmElements.row_cont[col], (void *)(uintptr_t)col);

        if (col == 0)
            lv_obj_add_event_cb(AlarmElements.row_cont[col], scroll_end_event_h, LV_EVENT_SCROLL_END, NULL);
        else
            lv_obj_add_event_cb(AlarmElements.row_cont[col], scroll_end_event_ms, LV_EVENT_SCROLL_END, NULL);

        lv_obj_scroll_to_y(AlarmElements.row_cont[col], (max_val / 2) * 40, LV_ANIM_ON);
    }
}

void LilyGoWatch::DrawAudioFace()
{

    // Create main container
    AudioGraphics.container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(AudioGraphics.container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(AudioGraphics.container, lv_color_black(), 0);
    lv_obj_set_style_border_color(AudioGraphics.container, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(AudioGraphics.container, 2, 0);
    lv_obj_set_style_bg_opa(AudioGraphics.container, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(AudioGraphics.container, 10, 0);
    lv_obj_set_scroll_dir(AudioGraphics.container, LV_DIR_NONE);

    // Create decorative circle background
    AudioGraphics.title_label = lv_label_create(AudioGraphics.container);
    lv_obj_t *circle_bg = lv_obj_create(AudioGraphics.container);
    lv_obj_set_size(circle_bg, LV_HOR_RES - 40, LV_HOR_RES - 40);
    lv_obj_align(circle_bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(circle_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle_bg, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_color(circle_bg, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(circle_bg, 1, 0);
    lv_obj_set_style_bg_opa(circle_bg, LV_OPA_COVER, 0);
    lv_obj_clear_flag(circle_bg, LV_OBJ_FLAG_CLICKABLE);

    // Title with enhanced styling
    lv_label_set_text(AudioGraphics.title_label, "AUDIO");
    lv_obj_set_style_text_font(AudioGraphics.title_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(AudioGraphics.title_label, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_align(AudioGraphics.title_label, LV_ALIGN_TOP_MID, 0, 15);

    // Create stylish buttons with rounded corners
    AudioGraphics.record_btn = lv_btn_create(AudioGraphics.container);
    lv_obj_set_size(AudioGraphics.record_btn, 110, 45);
    lv_obj_align(AudioGraphics.record_btn, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_radius(AudioGraphics.record_btn, 10, 0);
    lv_obj_set_style_bg_color(AudioGraphics.record_btn, lv_color_hex(0xDB4035), 0); // Softer red
    lv_obj_set_style_shadow_width(AudioGraphics.record_btn, 10, 0);
    lv_obj_set_style_shadow_color(AudioGraphics.record_btn, lv_color_hex(0x880000), 0);
    lv_obj_set_style_shadow_opa(AudioGraphics.record_btn, LV_OPA_50, 0);
    lv_obj_set_user_data(AudioGraphics.record_btn, (void *)6);
    lv_obj_add_event_cb(AudioGraphics.record_btn, ButtonHandler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *record_label = lv_label_create(AudioGraphics.record_btn);
    lv_label_set_text(record_label, "RECORD");
    lv_obj_set_style_text_font(record_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(record_label);

    AudioGraphics.play_btn = lv_btn_create(AudioGraphics.container);
    lv_obj_set_size(AudioGraphics.play_btn, 110, 45);
    lv_obj_align(AudioGraphics.play_btn, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_radius(AudioGraphics.play_btn, 10, 0);
    lv_obj_set_style_bg_color(AudioGraphics.play_btn, lv_color_hex(0x4CAF50), 0); // Softer green
    lv_obj_set_style_shadow_width(AudioGraphics.play_btn, 10, 0);
    lv_obj_set_style_shadow_color(AudioGraphics.play_btn, lv_color_hex(0x005500), 0);
    lv_obj_set_style_shadow_opa(AudioGraphics.play_btn, LV_OPA_50, 0);
    lv_obj_set_user_data(AudioGraphics.play_btn, (void *)5);
    lv_obj_add_event_cb(AudioGraphics.play_btn, ButtonHandler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *play_label = lv_label_create(AudioGraphics.play_btn);
    lv_label_set_text(play_label, "PLAY");
    lv_obj_set_style_text_font(play_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(play_label);
}

void LilyGoWatch::DrawIRWatchface()
{
    IRFace.container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(IRFace.container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(IRFace.container, lv_color_black(), 0);
    lv_obj_set_style_border_color(IRFace.container, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(IRFace.container, 2, 0);
    lv_obj_set_style_bg_opa(IRFace.container, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(IRFace.container, 10, 0);
    lv_obj_set_scroll_dir(IRFace.container, LV_DIR_NONE);

    lv_obj_t *circle_bg = lv_obj_create(IRFace.container);
    lv_obj_set_size(circle_bg, LV_HOR_RES - 40, LV_HOR_RES - 40);
    lv_obj_align(circle_bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(circle_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle_bg, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_color(circle_bg, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(circle_bg, 1, 0);
    lv_obj_set_style_bg_opa(circle_bg, LV_OPA_COVER, 0);
    lv_obj_clear_flag(circle_bg, LV_OBJ_FLAG_CLICKABLE);

    IRFace.title_label = lv_label_create(IRFace.container);
    lv_label_set_text(IRFace.title_label, "CONTROLS");
    lv_obj_set_style_text_font(IRFace.title_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(IRFace.title_label, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_align(IRFace.title_label, LV_ALIGN_TOP_MID, 0, 30);

    int btn_width = 60;
    int btn_height = 40;
    int h_spacing = 20;
    int v_spacing = 20;
    int start_y = 60;

    auto set_button_style = [](lv_obj_t *btn, uint32_t color, int id)
    {
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
        lv_obj_set_style_shadow_width(btn, 8, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(color), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
        lv_obj_set_user_data(btn, (void *)(intptr_t)id);
        lv_obj_add_event_cb(btn, ButtonHandler, LV_EVENT_CLICKED, NULL);
    };

    IRFace.btn1 = lv_btn_create(IRFace.container);
    lv_obj_set_size(IRFace.btn1, btn_width, btn_height);
    lv_obj_align(IRFace.btn1, LV_ALIGN_TOP_LEFT, 30, start_y);
    set_button_style(IRFace.btn1, 0x3498DB, 8);

    lv_obj_t *btn1_label = lv_label_create(IRFace.btn1);
    lv_label_set_text(btn1_label, "Vol. +");
    lv_obj_center(btn1_label);

    IRFace.btn2 = lv_btn_create(IRFace.container);
    lv_obj_set_size(IRFace.btn2, btn_width, btn_height);
    lv_obj_align(IRFace.btn2, LV_ALIGN_TOP_RIGHT, -30, start_y);
    set_button_style(IRFace.btn2, 0xE74C3C, 7);

    lv_obj_t *btn2_label = lv_label_create(IRFace.btn2);
    lv_label_set_text(btn2_label, "Power");
    lv_obj_center(btn2_label);

    IRFace.btn3 = lv_btn_create(IRFace.container);
    lv_obj_set_size(IRFace.btn3, btn_width, btn_height);
    lv_obj_align(IRFace.btn3, LV_ALIGN_TOP_LEFT, 30, start_y + btn_height + v_spacing);
    set_button_style(IRFace.btn3, 0x2ECC71, 9);

    lv_obj_t *btn3_label = lv_label_create(IRFace.btn3);
    lv_label_set_text(btn3_label, "Vol. -");
    lv_obj_center(btn3_label);

    IRFace.btn4 = lv_btn_create(IRFace.container);
    lv_obj_set_size(IRFace.btn4, btn_width, btn_height);
    lv_obj_align(IRFace.btn4, LV_ALIGN_TOP_RIGHT, -30, start_y + btn_height + v_spacing);
    set_button_style(IRFace.btn4, 0xF39C12, 10);

    lv_obj_t *btn4_label = lv_label_create(IRFace.btn4);
    lv_label_set_text(btn4_label, "Mute");
    lv_obj_center(btn4_label);

    lv_obj_t *center_circle = lv_obj_create(IRFace.container);
    lv_obj_set_size(center_circle, 30, 30);
    lv_obj_align(center_circle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(center_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(center_circle, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(center_circle, 0, 0);
    lv_obj_clear_flag(center_circle, LV_OBJ_FLAG_CLICKABLE);
}

void LilyGoWatch::SetDisplayBrightnessPercent(uint8_t value)
{
    if (value == 0) // Put display to sleep
    {
        disableALDO2();
        writecommand(0x10);
        XPowersCommon::writeRegister(FT6X36_REG_POWER_MODE, uint8_t(PMODE_MONITOR));
    }
    else if (PowerManage.Asleep) // Wake up display
    {
        enableALDO2();
        writecommand(0x11);
    }

    ledcWrite(LEDC_BACKLIGHT_CHANNEL, value); // Set display brightness
}
