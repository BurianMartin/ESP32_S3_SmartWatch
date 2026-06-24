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

        case SETTINGS_FACE:
            DrawSettingsFace();
            break;

        default:
            break;
        }
    }
}

void LilyGoWatch::DrawMainFace()
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);

    // --- Top bar: date (left), WiFi dot, battery bar + % (right) ---
    MainElements.date_label = lv_label_create(lv_scr_act());
    lv_label_set_text(MainElements.date_label, GetDateString());
    lv_obj_set_style_text_font(MainElements.date_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.date_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(MainElements.date_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // WiFi connected dot — green when connected, dark when not
    MainElements.wifi_conn_status_label = lv_obj_create(lv_scr_act());
    lv_obj_set_size(MainElements.wifi_conn_status_label, 8, 8);
    lv_obj_set_style_radius(MainElements.wifi_conn_status_label, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(MainElements.wifi_conn_status_label,
        WiFi.status() == WL_CONNECTED ? lv_color_hex(0x00CC00) : lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(MainElements.wifi_conn_status_label, 0, 0);
    lv_obj_set_style_shadow_width(MainElements.wifi_conn_status_label, 0, 0);
    lv_obj_clear_flag(MainElements.wifi_conn_status_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(MainElements.wifi_conn_status_label, MainElements.date_label, LV_ALIGN_OUT_RIGHT_MID, 6, 0);

    MainElements.battery_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(MainElements.battery_bar, 40, 10);
    lv_obj_align(MainElements.battery_bar, LV_ALIGN_TOP_RIGHT, -48, 14);
    lv_bar_set_value(MainElements.battery_bar, getBatteryPercent(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(MainElements.battery_bar,
        isCharging() ? lv_color_hex(0x00C800) : lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]),
        LV_PART_INDICATOR);

    MainElements.battery_label = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(MainElements.battery_label, "%d%%", getBatteryPercent());
    lv_obj_set_style_text_font(MainElements.battery_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.battery_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align_to(MainElements.battery_label, MainElements.battery_bar, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

    // --- Time centered on screen ---
    MainElements.time_label = lv_label_create(lv_scr_act());
    lv_label_set_text(MainElements.time_label, GetTimeString());
    lv_obj_set_style_text_font(MainElements.time_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(MainElements.time_label, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_align(MainElements.time_label, LV_ALIGN_CENTER, 0, -42);

    // --- Horizontal button row: Bright / WiFi / BT ---
    static const char *btn_labels[] = {"Bright", "WiFi", "BT"};
    static const int x_offsets[]   = {-68, 0, 68};
    for (int i = 0; i < 3; i++)
    {
        MainElements.buttons[i] = lv_btn_create(lv_scr_act());
        lv_obj_set_size(MainElements.buttons[i], 60, 42);
        lv_obj_set_style_radius(MainElements.buttons[i], 10, 0);
        lv_obj_align(MainElements.buttons[i], LV_ALIGN_CENTER, x_offsets[i], 8);
        lv_obj_add_event_cb(MainElements.buttons[i], ButtonHandler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(MainElements.buttons[i], (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(MainElements.buttons[i]);
        lv_label_set_text(lbl, btn_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(lbl);

        if ((i == 2 && MainElements.Bluetooth) || (i == 1 && Globals.wifi_mode != WIFI_MODE_NULL))
        {
            lv_obj_set_style_bg_color(MainElements.buttons[i], lv_color_hex(0x000000), LV_PART_MAIN);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
        else if (i == 0)
        {
            uint16_t rgb = 255 - (uint16_t)(DisplayBrightness * 2.55f);
            lv_obj_set_style_bg_color(MainElements.buttons[i], lv_color_make(rgb, rgb, rgb), LV_PART_MAIN);
            lv_obj_set_style_text_color(lbl,
                DisplayBrightness <= 50 ? lv_color_hex(0x000000) : lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_bg_color(MainElements.buttons[i], lv_color_hex(0xCECECE), LV_PART_MAIN);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), LV_PART_MAIN);
        }

        lv_obj_set_style_outline_color(MainElements.buttons[i], lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), LV_PART_MAIN);
        lv_obj_set_style_outline_width(MainElements.buttons[i], 2, LV_PART_MAIN);
        lv_obj_set_style_outline_pad(MainElements.buttons[i], 0, LV_PART_MAIN);
    }

    // --- Site button + status dot ---
    MainElements.site_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(MainElements.site_button, 130, 32);
    lv_obj_set_style_radius(MainElements.site_button, 10, 0);
    lv_obj_align(MainElements.site_button, LV_ALIGN_BOTTOM_LEFT, 38, -42);
    lv_obj_add_event_cb(MainElements.site_button, ButtonHandler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(MainElements.site_button, (void *)3);

    lv_obj_t *site_lbl = lv_label_create(MainElements.site_button);
    lv_label_set_text(site_lbl, Globals.website_on ? "Turn Site Off" : "Turn Site On");
    lv_obj_set_style_text_font(site_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(site_lbl);

    lv_obj_set_style_bg_color(MainElements.site_button, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_color(site_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_outline_color(MainElements.site_button, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), LV_PART_MAIN);
    lv_obj_set_style_outline_width(MainElements.site_button, 2, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(MainElements.site_button, 0, LV_PART_MAIN);

    // Site status dot (red/green) positioned to the left of the button
    MainElements.dot = lv_obj_create(lv_scr_act());
    lv_obj_set_size(MainElements.dot, 10, 10);
    lv_obj_set_style_radius(MainElements.dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(MainElements.dot, lv_color_hex(Globals.website_on ? 0x00FF00 : 0xFF0000), 0);
    lv_obj_set_style_border_width(MainElements.dot, 0, 0);
    lv_obj_set_style_shadow_width(MainElements.dot, 0, 0);
    lv_obj_clear_flag(MainElements.dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(MainElements.dot, MainElements.site_button, LV_ALIGN_OUT_LEFT_MID, -6, 0);

    DrawFaceIndicator();
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

    DrawFaceIndicator();
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
        lv_label_set_text(lock_label, LV_SYMBOL_EYE_CLOSE);
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

    // Column headers H / M / S
    static const char *col_headers[] = {"H", "M", "S"};
    for (uint8_t col = 0; col < 3; col++)
    {
        lv_obj_t *hdr = lv_label_create(AlarmElements.main_cont);
        lv_label_set_text(hdr, col_headers[col]);
        lv_obj_set_style_text_font(hdr, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(hdr, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
        lv_obj_set_pos(hdr, col * 80 + 32, 4);
        lv_obj_clear_flag(hdr, LV_OBJ_FLAG_CLICKABLE);
    }

    static lv_obj_t *number_labels[3][60];

    for (uint8_t col = 0; col < 3; col++)
    {
        AlarmElements.row_cont[col] = lv_obj_create(AlarmElements.main_cont);
        lv_obj_remove_style_all(AlarmElements.row_cont[col]);
        lv_obj_set_size(AlarmElements.row_cont[col], 80, 158);
        lv_obj_set_pos(AlarmElements.row_cont[col], col * 80, 22);

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

    DrawFaceIndicator();
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

    DrawFaceIndicator();
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

    DrawFaceIndicator();
}

void LilyGoWatch::DrawSettingsFace()
{
    static const char *color_names[] = {"Purple", "Yellow", "Red", "Green", "Blue"};
    static const uint32_t sleep_steps[] = {7, 10, 20, 30, 60};
    static const uint8_t sleep_step_count = 5;

    SettingsElements.container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(SettingsElements.container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(SettingsElements.container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(SettingsElements.container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(SettingsElements.container, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_pad_all(SettingsElements.container, 8, 0);
    lv_obj_set_scroll_dir(SettingsElements.container, LV_DIR_VER);

    // Title
    lv_obj_t *title = lv_label_create(SettingsElements.container);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    // Helper: creates a settings row — returns the row container
    auto make_row = [&](lv_obj_t *parent, const char *label_text, int y_offset) -> lv_obj_t *
    {
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_set_size(row, 220, 40);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x111111), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 4, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 30 + y_offset);
        lv_obj_set_scroll_dir(row, LV_DIR_NONE);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, label_text);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);
        return row;
    };

    // Helper: right-side value button
    auto make_val_btn = [&](lv_obj_t *row, const char *text, int btn_id) -> lv_obj_t *
    {
        lv_obj_t *btn = lv_btn_create(row);
        lv_obj_set_size(btn, 100, 32);
        lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -2, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_user_data(btn, (void *)(intptr_t)btn_id);
        lv_obj_add_event_cb(btn, ButtonHandler, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, text);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);
        return lbl;
    };

    // --- Row 0: Color ---
    lv_obj_t *row0 = make_row(SettingsElements.container, "Color", 0);

    SettingsElements.color_dot = lv_obj_create(row0);
    lv_obj_set_size(SettingsElements.color_dot, 14, 14);
    lv_obj_set_style_radius(SettingsElements.color_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(SettingsElements.color_dot, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(SettingsElements.color_dot, 0, 0);
    lv_obj_set_style_shadow_width(SettingsElements.color_dot, 0, 0);
    lv_obj_align(SettingsElements.color_dot, LV_ALIGN_RIGHT_MID, -110, 0);
    lv_obj_clear_flag(SettingsElements.color_dot, LV_OBJ_FLAG_CLICKABLE);

    SettingsElements.color_label = make_val_btn(row0, color_names[json_settings.gui_pref_color], 13);

    // --- Row 1: Time format ---
    lv_obj_t *row1 = make_row(SettingsElements.container, "Time", 46);
    SettingsElements.timeformat_label = make_val_btn(row1, json_settings.time_format.c_str(), 14);

    // --- Row 2: Sleep timeout ---
    lv_obj_t *row2 = make_row(SettingsElements.container, "Sleep", 92);

    lv_obj_t *dec_btn = lv_btn_create(row2);
    lv_obj_set_size(dec_btn, 30, 32);
    lv_obj_align(dec_btn, LV_ALIGN_RIGHT_MID, -72, 0);
    lv_obj_set_style_bg_color(dec_btn, lv_color_hex(0x222222), 0);
    lv_obj_set_style_radius(dec_btn, 6, 0);
    lv_obj_set_style_border_color(dec_btn, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(dec_btn, 1, 0);
    lv_obj_set_style_shadow_width(dec_btn, 0, 0);
    lv_obj_set_user_data(dec_btn, (void *)15);
    lv_obj_add_event_cb(dec_btn, ButtonHandler, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dec_lbl = lv_label_create(dec_btn);
    lv_label_set_text(dec_lbl, "-");
    lv_obj_set_style_text_color(dec_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(dec_lbl);

    SettingsElements.sleep_value_label = lv_label_create(row2);
    char sleep_buf[8];
    snprintf(sleep_buf, sizeof(sleep_buf), "%ds", PowerManage.sleep_timeout);
    lv_label_set_text(SettingsElements.sleep_value_label, sleep_buf);
    lv_obj_set_style_text_font(SettingsElements.sleep_value_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(SettingsElements.sleep_value_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(SettingsElements.sleep_value_label, LV_ALIGN_RIGHT_MID, -38, 0);

    lv_obj_t *inc_btn = lv_btn_create(row2);
    lv_obj_set_size(inc_btn, 30, 32);
    lv_obj_align(inc_btn, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_bg_color(inc_btn, lv_color_hex(0x222222), 0);
    lv_obj_set_style_radius(inc_btn, 6, 0);
    lv_obj_set_style_border_color(inc_btn, lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color]), 0);
    lv_obj_set_style_border_width(inc_btn, 1, 0);
    lv_obj_set_style_shadow_width(inc_btn, 0, 0);
    lv_obj_set_user_data(inc_btn, (void *)16);
    lv_obj_add_event_cb(inc_btn, ButtonHandler, LV_EVENT_CLICKED, NULL);
    lv_obj_t *inc_lbl = lv_label_create(inc_btn);
    lv_label_set_text(inc_lbl, "+");
    lv_obj_set_style_text_color(inc_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(inc_lbl);

    // --- Row 3: Date format ---
    lv_obj_t *row3 = make_row(SettingsElements.container, "Date fmt", 138);
    SettingsElements.dateformat_label = make_val_btn(row3, json_settings.date_format.c_str(), 17);

    // --- Row 4: Language ---
    lv_obj_t *row4 = make_row(SettingsElements.container, "Language", 184);
    SettingsElements.datelang_label = make_val_btn(row4, json_settings.date_language == "en" ? "EN" : "CZ", 18);

    DrawFaceIndicator();
}

void LilyGoWatch::DrawFaceIndicator()
{
    const int dot_size = 8;
    const int gap      = 6;
    const int count    = MAX_FACE - MIN_FACE + 1;
    const int total_w  = count * dot_size + (count - 1) * gap;
    const int start_x  = (LV_HOR_RES - total_w) / 2;

    for (int i = MIN_FACE; i <= MAX_FACE; i++)
    {
        lv_obj_t *dot = lv_obj_create(lv_scr_act());
        lv_obj_set_size(dot, dot_size, dot_size);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_shadow_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_set_style_bg_color(dot,
            (i == current_menu)
                ? lv_color_hex(HIGHLIGHT_COLORS[json_settings.gui_pref_color])
                : lv_color_hex(0x444444),
            0);
        lv_obj_set_pos(dot, start_x + (i - MIN_FACE) * (dot_size + gap), 228);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
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
