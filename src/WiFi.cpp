#include "LilyGoWatch.h"
#include "esp_wifi.h"

void LilyGoWatch::StartWiFi()
{
    wifi_.Start();
}

void LilyGoWatch::Add_Prefered_WiFi(std::string name, std::string pass)
{
    wifi_.AddPreferredNetwork(name, pass);
}

void LilyGoWatch::Remove_Prefered_WiFi(std::string name)
{
    wifi_.RemovePreferredNetwork(name);
}

void LilyGoWatch::StopWiFi()
{
    wifi_.Stop();
}

WiFiDriver::WiFiDriver(GeneralData &globals) : data(globals) {}

void WiFiDriver::Start()
{
    if (data.wifi_mode != data.active_wifi_mode)
    {
        if (xSemaphoreTake(data.wifi_mutex, portMAX_DELAY))
        {
            data.wifi_mode = data.active_wifi_mode;
            xSemaphoreGive(data.wifi_mutex);
        }

        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_MODE_NULL);
        WiFi.mode(data.wifi_mode);
    }

    switch (data.wifi_mode)
    {
    case WIFI_MODE_AP:
        xTaskCreatePinnedToCore(AccessPointTask, "AP Task", 4096, NULL, 1, NULL, 1);
        break;
    case WIFI_MODE_STA:
        data.scan_ticker.attach(WIFI_SCAN_REPEAT_TIME, []()
                                { xTaskCreatePinnedToCore(ScanTask, "WiFi Scan Task", 4096, NULL, 1, NULL, 1); });

        break;
    default:
        Serial.println("Invalid WiFi mode");
        break;
    }
}

void WiFiDriver::Stop()
{
    if (data.wifi_mode == WIFI_MODE_AP)
        WiFi.softAPdisconnect(true);

    data.scan_ticker.detach();
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.disconnect(true, true);

    if (xSemaphoreTake(data.wifi_mutex, portMAX_DELAY))
    {
        data.wifi_mode = WIFI_MODE_NULL;
        data.visible_networks = 0;
        data.scanResults.clear();
        xSemaphoreGive(data.wifi_mutex);
    }
}

void WiFiDriver::AddPreferredNetwork(const std::string &ssid, const std::string &pass)
{
    data.preferred_WiFis.emplace_back(ssid, pass);
}

void WiFiDriver::RemovePreferredNetwork(const std::string &ssid)
{
    data.preferred_WiFis.erase(std::remove_if(data.preferred_WiFis.begin(), data.preferred_WiFis.end(),
                                              [&ssid](const std::pair<std::string, std::string> &pair)
                                              {
                                                  return pair.first == ssid;
                                              }),
                               data.preferred_WiFis.end());
}

void WiFiDriver::AccessPointTask(void *param)
{
    WiFiDriver *self = Watch.getWifiDriverRef();

    if (WiFi.softAP(self->ap_ssid.c_str(), self->ap_password.c_str()))
    {
        Serial.print("Access Point Started. IP: ");
        Serial.println(WiFi.softAPIP());
    }
    else
        Serial.println("Failed to start AP");
    vTaskDelete(NULL);
}

void WiFiDriver::ScanTask(void *param)
{
    WiFiDriver *self = Watch.getWifiDriverRef();

    if (self->data.wifi_mode != WIFI_MODE_STA)
    {
        Serial.println("Wifi in wrong mode");
        vTaskDelete(NULL);
        return;
    }

    Serial.println("Starting Scan");

    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING)
    {
        Serial.println("Scan already running");
        vTaskDelete(NULL);
        return;
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi busy (connected to network), detaching periodic scan");
        self->data.scan_ticker.detach();
        vTaskDelete(NULL);
        return;
    }

    WiFi.scanNetworks(true, true); // async, show_hidden = true

    while (true)
    {
        int numNetworks = WiFi.scanComplete();

        if (numNetworks >= 0)
        {
            if (xSemaphoreTake(self->data.wifi_mutex, portMAX_DELAY))
            {
                Serial.printf("Found %d networks\n", numNetworks);

                self->data.visible_networks = 0;
                self->data.scanResults.clear();

                // Populate scanResults vector
                for (int i = 0; i < numNetworks; i++)
                {
                    String arduinoString = WiFi.SSID(i);
                    std::string ssid(arduinoString.c_str());

                    self->data.scanResults.push_back(WifiResult(
                        ssid,
                        WiFi.RSSI(i),
                        (WiFi.encryptionType(i) != WIFI_AUTH_OPEN)));
                }

                self->data.visible_networks = numNetworks;
                xSemaphoreGive(self->data.wifi_mutex);
            }
            WiFi.scanDelete();

            // Check if already connected
            bool connected = (WiFi.status() == WL_CONNECTED);

            if (!connected && self->data.preferred_WiFis.size() > 0)
            {
                // Find preferred networks in scan results
                std::vector<std::pair<int, int8_t>> preferred_found; // <index in preferred_WiFi, RSSI>

                for (int i = 0; i < (int)self->data.scanResults.size(); i++)
                {
                    const auto &network = self->data.scanResults[i];

                    for (int j = 0; j < (int)self->data.preferred_WiFis.size(); ++j)
                    {
                        if (network.ssid == self->data.preferred_WiFis[j].first)
                        {
                            preferred_found.push_back({j, network.signal_strength});
                            break;
                        }
                    }
                }

                std::sort(preferred_found.begin(), preferred_found.end(),
                          [](const std::pair<int, int8_t> &a, const std::pair<int, int8_t> &b)
                          {
                              return a.second > b.second;
                          });

                Serial.printf("Found %d preferred networks\n", (int)preferred_found.size());

                String prev_ip = WiFi.localIP().toString();

                for (const auto &p : preferred_found)
                {
                    Serial.printf("Trying to connect to %s with password '%s'\n", self->data.preferred_WiFis[p.first].first.c_str(), self->data.preferred_WiFis[p.first].second.c_str());

                    xEventGroupClearBits(self->data.wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
                    WiFi.begin(self->data.preferred_WiFis[p.first].first.c_str(), self->data.preferred_WiFis[p.first].second.c_str());

                    EventBits_t bits = xEventGroupWaitBits(
                        Watch.getWifiDriverRef()->data.wifi_events,
                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                        pdTRUE,
                        pdFALSE,
                        10000 / portTICK_PERIOD_MS);

                    if (bits & WIFI_CONNECTED_BIT)
                    {
                        Serial.println("Connected to WiFi");
                        Serial.print("IP: ");
                        Serial.println(WiFi.localIP().toString());
                        break;
                    }
                    else
                    {
                        wl_status_t status = WiFi.status();
                        Serial.print("Failed to connect. WiFi status: ");
                        Serial.println(status);

                        switch (status)
                        {
                        case WL_NO_SSID_AVAIL:
                            Serial.println("SSID not found.");
                            break;
                        case WL_CONNECT_FAILED:
                            Serial.println("Connection failed. Possibly wrong password?");
                            break;
                        case WL_CONNECTION_LOST:
                            Serial.println("Connection lost.");
                            break;
                        case WL_DISCONNECTED:
                            Serial.println("Disconnected.");
                            break;
                        default:
                            Serial.println("Unknown failure reason.");
                            break;
                        }
                    }
                }
            }
            break;
        }
        else if (numNetworks == 0)
            Serial.println("Scan finished, no networks found");
        else if (numNetworks == -2)
        {
            Serial.println("Scan was not started (mode or radio not ready)");
            break;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    if (self->data.wifi_mode == WIFI_MODE_AP)
        self->data.scan_ticker.detach();

    vTaskDelete(NULL);
}

void WiFiDriver::Event(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("Connected, got IP");
        xEventGroupSetBits(Watch.getWifiDriverRef()->data.wifi_events, WIFI_CONNECTED_BIT);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.print("Disconnected. Reason: ");
        Serial.println(info.wifi_sta_disconnected.reason);
        xEventGroupSetBits(Watch.getWifiDriverRef()->data.wifi_events, WIFI_FAIL_BIT);
        break;

    default:
        break;
    }
}

void WiFiDriver::SetApCreds(std::string ssid, std::string pass)
{
    if (xSemaphoreTake(data.wifi_mutex, portMAX_DELAY))
    {
        Serial.printf("ssid: %s, pass: %s\n", ssid.c_str(), pass.c_str());
        ap_ssid = ssid;
        ap_password = pass;
        xSemaphoreGive(data.wifi_mutex);
    }
}

std::string WiFiDriver::GetApSSID() const
{
    return ap_ssid;
}

std::string WiFiDriver::GetApPASS() const
{
    return ap_password;
}
