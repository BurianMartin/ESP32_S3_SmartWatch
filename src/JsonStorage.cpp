#include "LilyGoWatch.h"

JsonStorage::JsonStorage(const char *filename, size_t capacity)
    : filename(filename), capacity(capacity), doc(capacity) {}

bool JsonStorage::load(bool printErrors)
{
    File file = LittleFS.open(filename, "r");
    if (!file)
    {
        if (printErrors)
            Serial.println("Failed to open file for reading");
        return false;
    }
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err)
    {
        if (printErrors)
        {
            Serial.print("Deserialization failed: ");
            Serial.println(err.c_str());
        }
        return false;
    }
    return true;
}

bool JsonStorage::save(bool pretty)
{
    File file = LittleFS.open(filename, "w");
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return false;
    }
    size_t result = pretty ? serializeJsonPretty(doc, file) : serializeJson(doc, file);
    file.close();
    return result > 0;
}

String JsonStorage::getString(const char *key, const String &defaultValue)
{
    return doc[key] | defaultValue;
}

int JsonStorage::getInt(const char *key, int defaultValue)
{
    return doc[key] | defaultValue;
}

bool JsonStorage::getBool(const char *key, bool defaultValue)
{
    return doc[key] | defaultValue;
}

void JsonStorage::setString(const char *key, const String &value)
{
    doc[key] = value;
}

void JsonStorage::setInt(const char *key, int value)
{
    doc[key] = value;
}

void JsonStorage::setBool(const char *key, bool value)
{
    doc[key] = value;
}

JsonArray JsonStorage::getArray(const char *key)
{
    return doc[key].as<JsonArray>();
}

void JsonStorage::setArray(const char *key, const JsonArray &array)
{
    doc[key] = array;
}

std::string JsonStorage::getStdString(const char *key, const std::string &defaultValue)
{
    if (!doc.containsKey(key))
        return defaultValue;
    return std::string(doc[key].as<const char *>());
}

void JsonStorage::setStdString(const char *key, const std::string &value)
{
    doc[key] = value.c_str();
}
