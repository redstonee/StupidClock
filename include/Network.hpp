#pragma once

#include <WiFi.h>
#include <Preferences.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

namespace Network
{
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
