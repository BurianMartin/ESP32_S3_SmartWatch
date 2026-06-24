#pragma once
#include <esp_attr.h>

// Variables stored in RTC slow memory — survive deep sleep, zeroed on cold power-on.
// Defined in main.cpp; included by Power.cpp to write before esp_deep_sleep_start().
extern RTC_DATA_ATTR uint8_t rtc_display_brightness;
extern RTC_DATA_ATTR int16_t rtc_current_menu;
extern RTC_DATA_ATTR bool    rtc_deep_sleep_wake;
