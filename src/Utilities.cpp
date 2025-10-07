#include "Utilities.h"

const char *radio_freq_list =
    "433MHz\n"
    "470MHz\n"
    "868MHz\n"
    "915MHz\n"
    "923MHz";
const float radio_freq_args_list[] = {433.0, 470.0, 868.0, 915.0, 923.0};

const char *radio_bandwidth_list =
    "125KHz\n"
    "250KHz\n"
    "500KHz";
const float radio_bandwidth_args_list[] = {125.0, 250.0, 500.0};

const char *radio_power_level_list =
    "2dBm\n"
    "5dBm\n"
    "10dBm\n"
    "12dBm\n"
    "17dBm\n"
    "20dBm\n"
    "22dBm";
const float radio_power_args_list[] = {2, 5, 10, 12, 17, 20, 22};

const char *en_months[12] = {"Jan",
                             "Feb",
                             "Mar",
                             "Apr",
                             "May",
                             "Jun",
                             "Jul",
                             "Aug",
                             "Sep",
                             "Oct",
                             "Nov",
                             "Dec"};

const char *cz_months[12] = {"Led",
                             "Uno",
                             "Bre",
                             "Dub",
                             "Kve",
                             "Cer",
                             "Cvc",
                             "Srp",
                             "Zar",
                             "Rij",
                             "Lis",
                             "Pro"};

void ScanDevicePort(TwoWire *port, Stream *stream)
{
    uint8_t err, addr;
    int nDevices = 0;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            stream->printf("Device found at 0x%02X\n", addr);
        }
    }

    if (nDevices == 0)
        stream->print("No I2C devices were found!");
    else
        stream->print("Scan Done ");
}