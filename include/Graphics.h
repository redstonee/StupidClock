#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#include "Network.h"
#include "config.h"

extern SemaphoreHandle_t lvglMutex;

namespace Graphics {

    void updateNetworkStatus(Network::WiFiStatus status);

    void updateSaying(String saying);

    void closeMsgBox();

    void begin();
} // namespace Graphics
