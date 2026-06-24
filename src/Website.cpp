#include "LilyGoWatch.h"

void LilyGoWatch::HandleRoot()
{
    if (LittleFS.exists("/index.html"))
    {
        File html_index = LittleFS.open("/index.html", FILE_READ);
        if (html_index)
        {
            Website.streamFile(html_index, "text/html");
            html_index.close();
        }
        else
            Website.send(500, "text/plain", "Failed to open index.html");
    }
    else
        Website.send(404, "text/plain", "index.html not found");
}

void LilyGoWatch::HandleWifiGet()
{
    if (LittleFS.exists("/wifi.html"))
    {
        File html_wifi = LittleFS.open("/wifi.html", FILE_READ);
        if (html_wifi)
        {
            Website.streamFile(html_wifi, "text/html");
            html_wifi.close();
        }
        else
            Website.send(500, "text/plain", "Failed to open wifi.html");
    }
    else
        Website.send(404, "text/plain", "wifi.html not found");
}

void LilyGoWatch::HandleBrightness()
{
    if (Website.hasArg("value") && !PowerManage.Asleep)
    {
        DisplayBrightness = Website.arg("value").toInt();
        Serial.printf("Brightness form website: %d\n", DisplayBrightness);
        SetDisplayBrightnessPercent(DisplayBrightness);
    }
}

void LilyGoWatch::HandleSleepTimeoutSet()
{
    if (Website.hasArg("value"))
        PowerManage.SetSleepTimeout(Website.arg("value").toInt());
}

void LilyGoWatch::HandleWebsiteStop()
{
    Watch.StopServer();
    Watch.WebsiteUpdateTicker.detach();
}

void LilyGoWatch::HandleNetworkListGet()
{
    if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
    {
        if (Globals.scanResults.size() == 0)
            Website.send(200, "application/json", "[]");
        else
        {
            Serial.println("Sending netw list");

            String jsonResponse = "[";
            for (int i = 0; i < Globals.visible_networks; i++)
            {
                if (i > 0)
                    jsonResponse += ",";

                jsonResponse += "{";
                jsonResponse += "\"ssid\":\"" + String((Globals.scanResults[i].ssid).c_str()) + "\",";
                jsonResponse += "\"rssi\":" + String(Globals.scanResults[i].signal_strength) + ",";
                jsonResponse += "\"encryption\":" + String((Globals.scanResults[i].encrypted ? WIFI_AUTH_WEP : WIFI_AUTH_OPEN));
                jsonResponse += "}";
            }
            jsonResponse += "]";

            Website.send(200, "application/json", jsonResponse);
        }
        xSemaphoreGive(Watch.Globals.wifi_mutex);
    }
}

void LilyGoWatch::HandlePreferredListGet()
{

    Serial.println("Sending preferred network list");

    if (Watch.Globals.preferred_WiFis.empty())
    {
        Website.send(200, "application/json", "[]");
        return;
    }

    String jsonResponse = "[";
    for (size_t i = 0; i < Watch.Globals.preferred_WiFis.size(); i++)
    {
        if (i > 0)
            jsonResponse += ",";

        jsonResponse += "{";
        jsonResponse += "\"ssid\":\"" + String(Watch.Globals.preferred_WiFis[i].first.c_str()) + "\"";
        jsonResponse += "}";
    }
    jsonResponse += "]";

    Website.send(200, "application/json", jsonResponse);
}

void LilyGoWatch::HandleWifiSetMode()
{
    if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
    {
        if (Website.hasArg("mode"))
        {
            std::string mode = Website.arg("mode").c_str();

            if (mode == "AP")
            {
                Globals.active_wifi_mode = WIFI_MODE_AP;
                Globals.wifi_mode = Globals.active_wifi_mode;
            }
            else if (mode == "STA")
            {
                Globals.active_wifi_mode = WIFI_MODE_STA;
                Globals.wifi_mode = Globals.active_wifi_mode;
                Watch.StopServer();
                Watch.WebsiteUpdateTicker.detach();
                Watch.StartWiFi();
            }
            else if (mode == "NULL")
                Globals.wifi_mode = WIFI_MODE_NULL,
                Watch.StopWiFi(),
                Watch.StopServer(),
                Watch.WebsiteUpdateTicker.detach();

            WiFi.mode(Globals.wifi_mode);
        }
        xSemaphoreGive(Watch.Globals.wifi_mutex);
    }
}

void LilyGoWatch::HandleAddPreferredNetwork()
{
    Serial.println("Adding preferred network ... ");

    String ssid = "";
    String pass = "";

    if (Website.hasArg("ssid"))
        ssid = Website.arg("ssid");

    if (Website.hasArg("password"))
        pass = Website.arg("password");

    Serial.printf("ssid: %s, pass: %s\n", ssid.c_str(), pass.c_str());

    if (ssid != "" && pass != "")
    {
        Serial.printf("Adding prefferend network :: ssid: %s, pass: %s\n", ssid.c_str(), pass.c_str());
        Add_Prefered_WiFi(std::string(ssid.c_str()), std::string(pass.c_str()));
    }

    SaveSettingsToJson();
}

void LilyGoWatch::HandleRemovePreferredNetwork()
{
    Serial.println("Removing preferred network ... ");

    String ssid = "";

    if (Website.hasArg("ssid"))
        ssid = Website.arg("ssid");

    if (ssid != "")
    {
        Serial.printf("Removing prefferend network :: ssid: %s\n", ssid.c_str());
        Remove_Prefered_WiFi(std::string(ssid.c_str()));
    }
    SaveSettingsToJson();
}

void LilyGoWatch::SetApCredentials()
{
    String ssid = "";
    String pass = "";

    if (Website.hasArg("ssid"))
        ssid = Website.arg("ssid");

    if (Website.hasArg("password"))
        pass = Website.arg("password");

    Serial.printf("ssid: %s, pass: %s\n", ssid.c_str(), pass.c_str());

    std::string apssid = ssid.c_str();
    std::string appass = pass.c_str();

    if (ssid != "" && pass != "")
        wifi_.SetApCreds(apssid, appass);

    if (Watch.Globals.wifi_mode == WIFI_MODE_AP && xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
    {
        Watch.StopWiFi();
        Watch.Globals.wifi_mode = WIFI_MODE_NULL;
        Watch.StartWiFi();
        xSemaphoreGive(Watch.Globals.wifi_mutex);
    }
    SaveSettingsToJson();
}

void LilyGoWatch::SetupRoutes()
{
    Website.on("/", HTTP_GET, [this]()
               { this->HandleRoot(); });
    Website.on("/wifi", HTTP_GET, [this]()
               { this->HandleWifiGet(); });
    Website.on("/brightness", HTTP_POST, [this]()
               { this->HandleBrightness(); });
    Website.on("/color", HTTP_POST, [this]()
               { this->HandleColorSet(); });
    Website.on("/date", HTTP_POST, [this]()
               { this->HandleDateSet(); });
    Website.on("/sleep", HTTP_POST, [this]()
               { this->HandleSleepTimeoutSet(); });

    Website.on("/stop", HTTP_POST, [this]()
               { this->HandleWebsiteStop(); });
    Website.on("/wifi/networks", HTTP_GET, [this]()
               { this->HandleNetworkListGet(); });
    Website.on("/wifi/status", HTTP_GET, [this]()
               { this->HandleWifiStatus(); });
    Website.on("/wifi/mode", HTTP_POST, [this]()
               { this->HandleWifiSetMode(); });

    Website.on("/wifi/preferred/add", HTTP_POST, [this]()
               { this->HandleAddPreferredNetwork(); });

    Website.on("/wifi/preferred/remove", HTTP_POST, [this]()
               { this->HandleRemovePreferredNetwork(); });

    Website.on("/wifi/ap/set_credentials", HTTP_POST, [this]()
               { this->SetApCredentials(); });

    Website.on("/wifi/preferred/list", HTTP_GET, [this]()
               { this->HandlePreferredListGet(); });
}

void LilyGoWatch::HandleWifiStatus()
{
    String ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
    String json = "{\"mode\":" + String(Globals.wifi_mode) +
                  ",\"connected\":" + (WiFi.status() == WL_CONNECTED ? "true" : "false") +
                  ",\"ip\":\"" + ip + "\"}";
    Website.send(200, "application/json", json);
}

void LilyGoWatch::HandleColorSet()
{
    if (Website.hasArg("value"))
    {
        char color = Website.arg("value")[0];
        Serial.println(color);
        if (xSemaphoreTake(Watch.json_settings.Json_Acces_Mutex, portMAX_DELAY))
        {
            switch (color)
            {
            case 'P': // Purple
                json_settings.gui_pref_color = PURPLE;
                break;

            case 'Y': // Yellow
                json_settings.gui_pref_color = YELLOW;
                break;

            case 'R': // Red
                json_settings.gui_pref_color = RED;
                break;

            case 'G': // Green
                json_settings.gui_pref_color = GREEN;
                break;

            case 'B': // Blue
                json_settings.gui_pref_color = BLUE;
                break;

            default:
                break;
            }
            xSemaphoreGive(Watch.json_settings.Json_Acces_Mutex);
        }

        SaveSettingsToJson();
    }
}

void LilyGoWatch::HandleDateSet()
{
    int year = Website.arg("year").toInt();
    int month = Website.arg("month").toInt();
    int day = Website.arg("day").toInt();
    int hour = Website.arg("hour").toInt();
    int minute = Website.arg("minute").toInt();
    int second = Website.arg("second").toInt();

    Watch.setDateTime(year, month, day, hour, minute, second);
}

bool LilyGoWatch::StartServer()
{
    if (Globals.wifi_mode == WIFI_MODE_NULL)
        return false;

    if (!Globals.website_on && (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA))
    {
        SetupRoutes();
        Website.begin();
        Globals.website_on = true;
        return true;
    }
    else if (!Globals.website_on)
    {
        int timeout = 10;
        while (WiFi.status() != WL_CONNECTED && timeout > 0)
        {
            delay(500);
            timeout--;
        }

        if (WiFi.status() != WL_CONNECTED)
            return false;

        SetupRoutes();
        Website.begin();
        Serial.println("Before");
        if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
        {
            Globals.website_on = true;
            xSemaphoreGive(Watch.Globals.wifi_mutex);
        }
        Serial.println("After");
        return true;
    }
    return false;
}

void LilyGoWatch::StopServer()
{
    if (Globals.website_on)
    {
        Website.stop();
        if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
        {
            Globals.website_on = false;
            xSemaphoreGive(Watch.Globals.wifi_mutex);
        }
    }
}

void LilyGoWatch::SetWebsiteHandleFlag()
{
    if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
    {
        Watch.Globals.handle_website_events = true;
        xSemaphoreGive(Watch.Globals.wifi_mutex);
    }
}

void LilyGoWatch::HandleWebsite(void *parameter)
{
    if (xSemaphoreTake(Watch.Globals.wifi_mutex, portMAX_DELAY))
    {
        Watch.Globals.handle_website_events = false;
        xSemaphoreGive(Watch.Globals.wifi_mutex);
    }
    Watch.Website.handleClient();
    Watch.Globals.website_task_handle = NULL;
    vTaskDelete(NULL);
}
