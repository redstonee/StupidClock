#include <ArduinoJson.hpp>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "Graphics.h"
#include "Network.h"
#include "RTC.h"
#include "config.h"

namespace Network {

    WiFiStatus wifiStatus = WiFiStatus::DISCONNECTED;

    void init() {
        WiFi.begin();

        xTaskCreate(checkWiFiTask, "Check WiFi Task", 2048, nullptr, 1,
                    nullptr);

        xTaskCreate(ntpTask, "NTP Task", 4096, nullptr, 1, nullptr);

        // xTaskCreate(getSayingTask, "Get Saying Task", 32768, nullptr, 1,
        //             nullptr);
    }

    void setWiFiStatus(WiFiStatus status) {
        wifiStatus = status;
        Graphics::updateNetworkStatus(status);
    }

    void checkWiFiTask(void *pvParameters) {
        while (1) {
            if (WiFi.status() == WL_CONNECTED)
                setWiFiStatus(WiFiStatus::CONNECTED);
            else if (getWiFiStatus() != WiFiStatus::CONNECTING)
                setWiFiStatus(WiFiStatus::DISCONNECTED);

            delay(500);
        }
    }

    void smartConfigTask(void *pvParameters) {
        wifiStatus = WiFiStatus::CONNECTING;
        WiFi.beginSmartConfig();

        Serial.println("Waiting for SmartConfig.");

        auto startTime = millis();
        while (millis() - startTime < SMART_CONFIG_TIMEOUT_MS) {
            if (WiFi.smartConfigDone()) {
                WiFi.setAutoConnect(true);
                WiFi.setAutoReconnect(true);
                Serial.println("SmartConfig received.");
                break;
            }
            delay(1000);
        }

        WiFi.stopSmartConfig();
        Graphics::closeMsgBox();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("SmartConfig failed.");
            WiFi.disconnect();
            setWiFiStatus(WiFiStatus::DISCONNECTED);
        }

        vTaskDelete(nullptr);
    }
    void ntpTask(void *shit) {
        WiFiUDP wifiUDP;
        NTPClient ntpClient(wifiUDP, "ntp.aliyun.com", 3600 * 8, 60000);
        ntpClient.begin();

        while (1) {
            delay(10000);
            if (getWiFiStatus() != WiFiStatus::CONNECTED)
                continue;

            if (ntpClient.update()) { // Will be true if update successfully
                RTC::setDateTime(ntpClient.getEpochTime());
            }
        }
    }

    void getSayingTask(void *shit) {
        HTTPClient httpClient;
        httpClient.begin(oneSayingURL);

        while (1) {
            delay(1e4);
            if (getWiFiStatus() != WiFiStatus::CONNECTED)
                continue;

            using namespace ArduinoJson;
            auto code = httpClient.GET();
            if (code = HTTP_CODE_OK) {
                auto response = httpClient.getString();
                Serial.println(response);

                JsonDocument doc;
                deserializeJson(doc, response);
                JsonObject data = doc["data"];

                Graphics::updateSaying(data["content"].as<String>() + " - " +
                                       data["origin"].as<String>());

                continue;
            }

            Serial.printf("HTTP fucked up with code %d\n", code);
            Graphics::updateSaying("获取失败");
        }
    }

    void removeNetwork() {
        WiFi.setAutoConnect(false);
        WiFi.setAutoReconnect(false);
        WiFi.disconnect();
    }

} // namespace Network
