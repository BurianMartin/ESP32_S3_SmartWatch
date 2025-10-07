#include "LilyGoWatch.h"

bool LilyGoWatch::CreateFileWAV(const char *song_name, uint32_t duration, uint16_t num_channels, const uint32_t sampling_rate, uint16_t bits_per_sample)
{
    uint32_t data_size = sampling_rate * num_channels * bits_per_sample * duration / 8;
    uint32_t byte_rate = sampling_rate * num_channels * bits_per_sample / 8;
    uint16_t block_align = num_channels * bits_per_sample / 8;
    uint32_t chunk_size = data_size + 36;

    File new_audio_file = LittleFS.open(song_name, FILE_WRITE);
    if (!new_audio_file)
    {
        Serial.println("Failed to create file");
        return false;
    }

    auto writeLE = [&new_audio_file](uint32_t value, size_t bytes)
    {
        for (size_t i = 0; i < bytes; i++)
            new_audio_file.write((uint8_t)(value >> (i * 8)));
    };

    auto writeString = [&new_audio_file](const char *str)
    {
        new_audio_file.write((const uint8_t *)str, strlen(str));
    };

    writeString("RIFF");
    writeLE(chunk_size, 4);
    writeString("WAVE");

    writeString("fmt ");
    writeLE(16, 4);
    writeLE(1, 2);
    writeLE(num_channels, 2);
    writeLE(sampling_rate, 4);
    writeLE(byte_rate, 4);
    writeLE(block_align, 2);
    writeLE(bits_per_sample, 2);

    writeString("data");
    writeLE(data_size, 4);

    new_audio_file.close();
    return true;
}

void LilyGoWatch::LoadSettingsFromJson(bool printData)
{
    JsonStorage jsonDriver("/settings.json");

    if (!jsonDriver.load())
        return;

    if (xSemaphoreTake(json_settings.Json_Acces_Mutex, portMAX_DELAY))
    {
        wifi_.SetApCreds(jsonDriver.getString("ap_ssid", "LilyGoWatch_AP").c_str(), jsonDriver.getString("ap_pass", "GingerBreadMan").c_str());
        json_settings.time_format = jsonDriver.getString("time_format", "24h").c_str();
        json_settings.date_format = jsonDriver.getString("date_format", "DD-MM-YYYY").c_str();
        json_settings.date_language = jsonDriver.getString("date_language", "en").c_str();
        uint32_t sleep_timeout = jsonDriver.getInt("sleep_timeout", SLEEP_TIMEOUT);
        PowerManage.SetSleepTimeout(sleep_timeout);

        json_settings.gui_pref_color = GuiColor(jsonDriver.getInt("gui_color", 0));

        Globals.preferred_WiFis.clear();
        xSemaphoreGive(json_settings.Json_Acces_Mutex);
    }

    JsonArray arr = jsonDriver.getArray("preferred_networks");
    for (JsonObject net : arr)
        Add_Prefered_WiFi(net["ssid"] | "", net["password"] | "");

    if (printData)
    {
        Serial.println("Loaded settings from JSON:");
        Serial.printf("  AP credentials:        %s / %s\n", wifi_.GetApSSID().c_str(), wifi_.GetApPASS().c_str());
        Serial.printf("  Time format:           %s\n", json_settings.time_format.c_str());
        Serial.printf("  Date format:           %s\n", json_settings.date_format.c_str());
        Serial.printf("  Date language:         %s\n", json_settings.date_language.c_str());
        Serial.printf("  Sleep timeout:         %lu seconds\n", static_cast<unsigned long>(PowerManage.sleep_timeout));
        Serial.printf("  GUI preferred color:   %d\n", static_cast<int>(json_settings.gui_pref_color));

        if (Globals.preferred_WiFis.empty())
            Serial.println("  Preferred networks:    none");
        else
        {
            Serial.println("  Preferred networks:");
            for (const auto &p : Globals.preferred_WiFis)
                Serial.printf("    - SSID: %s, Pass: %s\n", p.first.c_str(), p.second.c_str());
        }
    }
}

void LilyGoWatch::SaveSettingsToJson()
{
    JsonStorage jsonDriver("/settings.json");

    if (xSemaphoreTake(json_settings.Json_Acces_Mutex, portMAX_DELAY))
    {
        jsonDriver.setStdString("ap_ssid", wifi_.GetApSSID().c_str());
        jsonDriver.setStdString("ap_pass", wifi_.GetApSSID().c_str());
        jsonDriver.setStdString("time_format", json_settings.time_format);
        jsonDriver.setStdString("date_format", json_settings.date_format);
        jsonDriver.setStdString("date_language", json_settings.date_language);
        jsonDriver.setInt("sleep_timeout", PowerManage.sleep_timeout);
        jsonDriver.setInt("gui_color", json_settings.gui_pref_color);
        xSemaphoreGive(json_settings.Json_Acces_Mutex);
    }

    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.to<JsonArray>();
    for (auto &pair : Globals.preferred_WiFis)
    {
        JsonObject net = arr.createNestedObject();
        net["ssid"] = pair.first;
        net["password"] = pair.second;
    }

    jsonDriver.setArray("preferred_networks", arr);
    jsonDriver.save();
}
