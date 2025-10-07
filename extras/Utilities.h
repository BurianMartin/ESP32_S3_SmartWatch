#pragma once
#include <lvgl.h>
#include <Wire.h>
#include <iostream>

#define LVGL_USE 1
#undef DEBUG
#undef ACCELEROMETER
#define INTRO_DELAY 0

#define BOARD_TFT_WIDTH (240)
#define BOARD_TFT_HEIHT (240)

// ST7789
#define BOARD_TFT_MISO (-1)
#define BOARD_TFT_MOSI (13)
#define BOARD_TFT_SCLK (18)
#define BOARD_TFT_CS (12)
#define BOARD_TFT_DC (38)
#define BOARD_TFT_RST (-1)
#define BOARD_TFT_BL (45)

// Touch
#define BOARD_TOUCH_SDA (39)
#define BOARD_TOUCH_SCL (40)
#define BOARD_TOUCH_INT (16)

// BMA423,PCF8563,AXP2101,DRV2605L
#define BOARD_I2C_SDA (10)
#define BOARD_I2C_SCL (11)

// PCF8563 Interrupt
#define BOARD_RTC_INT_PIN (17)
// AXP2101 Interrupt
#define BOARD_PMU_INT (21)
// BMA423 Interrupt
#define BOARD_BMA423_INT1 (14)
#define BMA_ADDR 0x19

// IR Transmission
#define BOARD_IR_PIN (2)

// MAX98357A
#define BOARD_DAC_IIS_BCK (48)
#define BOARD_DAC_IIS_WS (15)
#define BOARD_DAC_IIS_DOUT (46)

// SX1262 Radio Pins
#define BOARD_RADIO_SCK (3)
#define BOARD_RADIO_MISO (4)
#define BOARD_RADIO_MOSI (1)
#define BOARD_RADIO_SS (5)
#define BOARD_RADIO_DI01 (9)
#define BOARD_RADIO_RST (8)
#define BOARD_RADIO_BUSY (7)
#define BOARD_RADIO_DI03 (6)

// PDM Microphone
#define BOARD_MIC_DATA (47)
#define BOARD_MIC_CLOCK (44)

#define SHIELD_GPS_TX (42)
#define SHIELD_GPS_RX (41)

#define WATCH_RADIO_ONLINE _BV(0)
#define WATCH_TOUCH_ONLINE _BV(1)
#define WATCH_DRV_ONLINE _BV(2)
#define WATCH_PMU_ONLINE _BV(3)
#define WATCH_RTC_ONLINE _BV(4)
#define WATCH_BMA_ONLINE _BV(5)
#define WATCH_GPS_ONLINE _BV(6)

#define LEDC_BACKLIGHT_CHANNEL 3
#define LEDC_BACKLIGHT_BIT_WIDTH 8
#define LEDC_BACKLIGHT_FREQ 1000

// !The PDM microphone can only be up to 16KHZ and cannot be changed
#define MIC_I2S_SAMPLE_RATE 16000
// !The PDM microphone can only use I2S channel 0 and cannot be changed
#define MIC_I2S_PORT I2S_NUM_0
#define MIC_I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT

enum PowerMode
{
    PMODE_ACTIVE = 0,    // ~4mA
    PMODE_MONITOR = 1,   // ~3mA
    PMODE_DEEPSLEEP = 3, // ~100uA  The reset pin must be pulled down to wake up
};

#define RADIO_DEFAULT_FREQ 868.0
#define RADIO_DEFAULT_BW 250.0
#define RADIO_DEFAULT_SF 10
#define RADIO_DEFAULT_CR 6
#define RADIO_DEFAULT_CUR_LIMIT 140
#define RADIO_DEFAULT_POWER_LEVEL 22

#define SLEEP_TIMEOUT 7
#define WIFI_SCAN_REPEAT_TIME 10

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define rgbColor(r, g, b) (((r >> 3) & 0x1F) << 11 | ((g >> 2) & 0x3F) << 5 | ((b >> 3) & 0x1F))
#define rgbToHex(r, g, b) ((r << 16) | (g << 8) | b)

extern const char *radio_freq_list;
extern const float radio_freq_args_list[];
extern const char *radio_bandwidth_list;
extern const float radio_bandwidth_args_list[];
extern const char *radio_power_level_list;
extern const float radio_power_args_list[];
extern const char *en_months[12];
extern const char *cz_months[12];

enum SwipeDirection
{
    NO_SWIPE = 0,
    SWIPE_UP = 1,
    SWIPE_RIGHT = 2,
    SWIPE_DOWN = 3,
    SWIPE_LEFT = 4
};

enum GuiColor // gui_pref_color
{
    PURPLE = 0,
    YELLOW = 1,
    RED = 2,
    GREEN = 3,
    BLUE = 4
};

void ScanDevicePort(TwoWire *port, Stream *stream);

#define LVGL_HIGHLIGHT_COLOR_RED 0xED1F1F
#define LVGL_HIGHLIGHT_COLOR_GREEN 0x43FF3D
#define LVGL_HIGHLIGHT_COLOR_BLUE 0x2D1FED
#define LVGL_HIGHLIGHT_COLOR_YELLOW 0xECFF3D
#define LVGL_HIGHLIGHT_COLOR_PURPLE 0xAF3DFF

const int HIGHLIGHT_COLORS[] = {
    0xAF3DFF,  // Purple
    0xECFF3D,  // Yellow
    0xED1F1F,  // Red
    0x43FF3D,  // Green
    0x2D1FED}; // Blue

#define MIN_FACE 0
#define MAX_FACE 3

#define IR_FACE 0
#define WIFI_FACE 1
#define MAIN_FACE 2
#define SPEAKER_FACE 3
#define ALARM_FACE 4

#define DEFAULT_RECORD_FILENAME "/rec.wav"