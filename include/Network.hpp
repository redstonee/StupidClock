#pragma once

#include <WiFi.h>
#include <Preferences.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include "config.h"

namespace Network
{
    static WiFiUDP wifiUDP;
    static NTPClient ntpClient(wifiUDP, "ntp.aliyun.com", 3600 * 8, 60000);

    enum class WiFiStatus
    {
        CONNECTED,
        DISCONNECTED,
        CONNECTING
    };

    static WiFiStatus wifiStatus = WiFiStatus::DISCONNECTED;

    inline WiFiStatus getWiFiStatus()
    {
        return wifiStatus;
    }

    void setWiFiStatus(WiFiStatus status)
    {
        wifiStatus = status;
    }

    inline void checkWiFiTask(void *pvParameters)
    {
        while (1)
        {
            if (WiFi.status() == WL_CONNECTED)
                setWiFiStatus(WiFiStatus::CONNECTED);
            else if (getWiFiStatus() != WiFiStatus::CONNECTING)
                setWiFiStatus(WiFiStatus::DISCONNECTED);

            delay(500);
        }
    }

    void smartConfigTask(void *pvParameters)
    {
        wifiStatus = WiFiStatus::CONNECTING;
        WiFi.beginSmartConfig();
        auto startTime = millis();
        while (millis() - startTime < SMART_CONFIG_TIMEOUT_MS)
        {
            if (WiFi.smartConfigDone())
            {
                WiFi.setAutoConnect(true);
                WiFi.setAutoReconnect(true);
                WiFi.stopSmartConfig();
                break;
            }
            delay(1000);
        }
        if (WiFi.status() != WL_CONNECTED)
        {
            WiFi.disconnect();
            setWiFiStatus(WiFiStatus::DISCONNECTED);
        }
    }

    void ntpTask(void *shit)
    {

        while (1)
        {
#ifdef DEBUGING
            Serial.printf("ntp:");
            Serial.println(DateTime.toISOString());
#endif

            delay(10000);
        }
    }
} // namespace Network
