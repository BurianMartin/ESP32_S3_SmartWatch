#include "LilyGoWatch.h"

LilyGoWatch Watch;

void LilyGoWatch::NextMenu()
{
    current_menu = (current_menu == MAX_FACE ? MIN_FACE : current_menu + 1);
}

void LilyGoWatch::PrevMenu()
{
    current_menu = (current_menu == MIN_FACE ? MAX_FACE : current_menu - 1);
}

void LilyGoWatch::Print(const char *message)
{
    if (stream)
        stream->print(message);
}

void LilyGoWatch::PrintLn(const char *message)
{
    if (stream)
        stream->println(message);
}

void LilyGoWatch::ScanDevicePort(TwoWire *port, Stream *stream)
{
    int nDevices = 0;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        port->beginTransmission(addr);
        if (port->endTransmission() == 0)
        {
            if (stream) stream->printf("Device found at 0x%02X\n", addr);
            nDevices++;
        }
    }

    if (nDevices == 0)
        PrintLn("No I2C devices were found!");
    else
        PrintLn("Scan Done ");
}

const char *LilyGoWatch::GetTimeString()
{
    RTC_DateTime dateTime = getDateTime();

    static char time[6];

    if (json_settings.time_format == "12h")
    {
        int hour12 = dateTime.hour % 12;
        if (hour12 == 0)
            hour12 = 12;

        snprintf(time, sizeof(time), "%02d:%02d", hour12, dateTime.minute);
    }
    else
        snprintf(time, sizeof(time), "%02d:%02d", dateTime.hour, dateTime.minute);

    return time;
}

const char *LilyGoWatch::GetDateString()
{
    RTC_DateTime dateTime = getDateTime();

    static char date[15];

    const char **months;

    if (json_settings.date_language == "en")
        months = en_months;
    else if (json_settings.date_language == "cz")
        months = cz_months;
    else
        months = en_months;

    if (json_settings.date_format == "DD-MM-YYYY")
        snprintf(date, sizeof(date), "%02d %s %04d", dateTime.day, months[dateTime.month - 1], dateTime.year);
    else if (json_settings.date_format == "YYYY-MM-DD")
        snprintf(date, sizeof(date), "%04d %s %02d", dateTime.year, months[dateTime.month - 1], dateTime.day);
    else
        snprintf(date, sizeof(date), "%d %s %d", dateTime.day, months[dateTime.month - 1], dateTime.year);

    return date;
}

uint32_t LilyGoWatch::ReadRegister(int reg)
{
    return TouchDrvFT6X36::readRegister(reg);
}
