#pragma once

#include <NTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"

namespace Network {

    enum class WiFiStatus { CONNECTED, DISCONNECTED, CONNECTING };

    void init();

    extern WiFiStatus wifiStatus;

    inline WiFiStatus getWiFiStatus() { return wifiStatus; }

    void setWiFiStatus(WiFiStatus status);

    void checkWiFiTask(void *pvParameters);

    void smartConfigTask(void *pvParameters);

    void ntpTask(void *shit);

    void getSayingTask(void *shit);

    void removeNetwork();

    String getOneSaying();

} // namespace Network
