#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#include "Network.h"
#include "config.h"

namespace GFXDriver {

    void display_flush(lv_display_t *disp, const lv_area_t *area,
                       uint8_t *px_map);

    void touch_read(lv_indev_t *indev, lv_indev_data_t *data);

#if LV_USE_LOG != 0
    /* Serial debugging */
    void log_print(lv_log_level_t level, const char *buf);
#endif

    void init();

} // namespace GFXDriver

extern SemaphoreHandle_t lvglMutex;

namespace Graphics {

    inline void initDriver() {
        GFXDriver::init();
        lvglMutex = xSemaphoreCreateMutex();
    }

    void updateNetworkStatus(Network::WiFiStatus status);

    void updateSaying(String saying);

    void closeMsgBox();

    void begin();
} // namespace Graphics
