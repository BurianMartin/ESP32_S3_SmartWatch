#include "LilyGoWatch.h"

void LilyGoWatch::PlayIrSignal(void *parameter)
{
    int ID = (int)(intptr_t)parameter;

    File file = LittleFS.open("/settings.json", "r");
    if (!file)
    {
        Serial.println("Failed to open settings.json");
        vTaskDelete(NULL);
        return;
    }

    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        vTaskDelete(NULL);
        return;
    }

    String manufacturer = doc["Infrared"]["manufacturer"] | "Unknown Manufacturer";
    Serial.println("Manufacturer: " + manufacturer);

    JsonArray signals = doc["Infrared"]["signals"].as<JsonArray>();

    if (ID < 0 || ID >= (int)signals.size())
    {
        Serial.println("Invalid signal ID.");
        vTaskDelete(NULL);
        return;
    }

    uint64_t sig = strtoull(signals[ID].as<String>().c_str(), nullptr, 16);

    if (manufacturer == "Samsung")
        Watch.sendSAMSUNG(sig, 32);
    else if (manufacturer == "LG")
        Watch.sendLG(sig, 48);
    else if (manufacturer == "Sony")
        Watch.sendSony(sig, 20);
    else if (manufacturer == "NEC")
        Watch.sendNEC(sig, 32);
    else if (manufacturer == "Mitsubishi")
        Watch.sendMitsubishi(sig, 64);
    else
        Serial.println("Unsupported manufacturer.");

    vTaskDelete(NULL);
}
