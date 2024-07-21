#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#include "Graphics.h"
#include "Network.h"
#include "RTC.h"
#include "config.h"

extern "C" {
LV_IMG_DECLARE(wifi_connected)
LV_IMG_DECLARE(wifi_disconnected)
}

SemaphoreHandle_t lvglMutex;

namespace GFXDriver {
    TFT_eSPI tft;

    void display_flush(lv_display_t *disp, const lv_area_t *area,
                       uint8_t *px_map) {
        uint32_t w = lv_area_get_width(area);
        uint32_t h = lv_area_get_height(area);

        tft.startWrite();
        tft.setAddrWindow(area->x1, area->y1, w, h);
        tft.pushPixelsDMA((uint16_t *)px_map, w * h);
        tft.endWrite();

        lv_disp_flush_ready(disp);
    }

    void touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
        uint16_t touchX, touchY;

        bool touched = tft.getTouch(&touchX, &touchY, 600);

        if (!touched) {
            data->state = LV_INDEV_STATE_REL;
        } else {
            data->state = LV_INDEV_STATE_PR;

            /*Set the coordinates*/
            data->point.x = touchX;
            data->point.y = touchY;
        }
    }

#if LV_USE_LOG != 0
    /* Serial debugging */
    void log_print(lv_log_level_t level, const char *buf) {
        LV_UNUSED(level);
        Serial.printf(buf);
        Serial.flush();
    }
#endif

    void init() {
        tft.begin();
        tft.setRotation(1);
        tft.setSwapBytes(true);
        tft.initDMA();
        uint16_t calData[5] = {383, 3506, 311, 3408, 3};
        tft.setTouch(calData);

        lv_init();
        lv_tick_set_cb([]() { return (uint32_t)millis(); });

#if LV_USE_LOG != 0
        lv_log_register_print_cb(log_print);
#endif

        static uint32_t draw_buf[LCD_WIDTH * LCD_HEIGHT / 40];
        auto disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
        lv_display_set_flush_cb(disp, display_flush);
        lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
                               LV_DISPLAY_RENDER_MODE_PARTIAL);

        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(
            indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
        lv_indev_set_read_cb(indev, touch_read);
    }

} // namespace GFXDriver

namespace Graphics {

    lv_obj_t *networkBtn;
    lv_obj_t *timeLabel;
    lv_obj_t *dateLabel;
    lv_obj_t *dayOfWeekLabel;
    lv_obj_t *sayingLabel;
    lv_obj_t *wifiMsgBox;

    void startSmartconfig(lv_event_t *e) {
        // Remove the close button if exists
        auto header = lv_msgbox_get_header(wifiMsgBox);
        if (lv_obj_get_child_count_by_type(header,
                                           &lv_msgbox_header_button_class))
            lv_obj_delete(lv_obj_get_child_by_type(
                header, 0, &lv_msgbox_header_button_class));

        // Change the button for canceling
        auto btn = lv_event_get_target_obj(e);
        lv_label_set_text(lv_obj_get_child_by_type(btn, 0, &lv_label_class),
                          "Cancel");

        static const auto cancel = [](lv_event_t *e) {
            WiFi.stopSmartConfig();
            closeMsgBox();
        };

        lv_obj_add_event_cb(btn, cancel, LV_EVENT_CLICKED, nullptr);

        auto circle = lv_spinner_create(lv_msgbox_get_content(wifiMsgBox));
        lv_obj_set_size(circle, 48, 48);
        xTaskCreate(Network::smartConfigTask, "SmartConfigTask", 4096,
        nullptr, 1, nullptr);
    }

    void closeMsgBox() { lv_msgbox_close(wifiMsgBox); }

    void networkButtonHandler(lv_event_t *e) {
        wifiMsgBox = lv_msgbox_create(lv_scr_act());
        lv_msgbox_add_title(wifiMsgBox, "WiFi Settings");
        lv_msgbox_add_close_button(wifiMsgBox);

        if (Network::getWiFiStatus() == Network::WiFiStatus::DISCONNECTED) {
            lv_msgbox_add_text(wifiMsgBox, "WiFi disconnected");

            auto buttonStart =
                lv_msgbox_add_footer_button(wifiMsgBox, "Start Smartconfig");

            lv_obj_add_event_cb(buttonStart, startSmartconfig, LV_EVENT_CLICKED,
                                nullptr);

        } else if (Network::getWiFiStatus() == Network::WiFiStatus::CONNECTED) {
            lv_msgbox_add_text(wifiMsgBox, "WiFi connected");
            lv_msgbox_add_text(wifiMsgBox,
                               ("IP: " + WiFi.localIP().toString()).c_str());

            auto buttonDisconnect =
                lv_msgbox_add_footer_button(wifiMsgBox, "Disconnect");

            lv_obj_add_event_cb(
                buttonDisconnect,
                [](lv_event_t *e) {
                    Network::removeNetwork();
                    closeMsgBox();
                },
                LV_EVENT_CLICKED, nullptr);
        }
    }

    void updateDateTimeStringTask(void *pvParameters) {
        while (true) {
            auto formattedTime = RTC::getTimeString();
            auto formattedDate = RTC::getDateString();
            auto dayOfWeek = RTC::getDayOfWeekString();
            if (xSemaphoreTake(lvglMutex, 100 / portTICK_PERIOD_MS)) {
                lv_label_set_text(timeLabel, formattedTime.c_str());
                lv_label_set_text(dateLabel, formattedDate.c_str());
                lv_label_set_text(dayOfWeekLabel, dayOfWeek.c_str());
                xSemaphoreGive(lvglMutex);
            }
            delay(500);
        }
    }

    void updateNetworkStatus(Network::WiFiStatus status) {
        static const auto removeSpinner = []() {
            auto spinnerCount =
                lv_obj_get_child_count_by_type(networkBtn, &lv_spinner_class);
            if (spinnerCount)
                for (uint8_t i = 0; i < spinnerCount; i++)
                    lv_obj_delete(lv_obj_get_child_by_type(networkBtn, i,
                                                           &lv_spinner_class));
        };
        if (xSemaphoreTake(lvglMutex, 100 / portTICK_PERIOD_MS)) {
            switch (status) {
            case Network::WiFiStatus::CONNECTED:
                lv_imagebutton_set_state(networkBtn,
                                         LV_IMAGEBUTTON_STATE_RELEASED);
                lv_imagebutton_set_src(networkBtn,
                                       LV_IMAGEBUTTON_STATE_RELEASED, nullptr,
                                       &wifi_connected, nullptr);
                removeSpinner();
                break;

            case Network::WiFiStatus::DISCONNECTED:
                lv_imagebutton_set_state(networkBtn,
                                         LV_IMAGEBUTTON_STATE_RELEASED);
                lv_imagebutton_set_src(networkBtn,
                                       LV_IMAGEBUTTON_STATE_RELEASED, nullptr,
                                       &wifi_disconnected, nullptr);
                removeSpinner();
                break;

            case Network::WiFiStatus::CONNECTING:
                lv_imagebutton_set_state(networkBtn,
                                         LV_IMAGEBUTTON_STATE_DISABLED);
                break;
            }
            xSemaphoreGive(lvglMutex);
        }
    }

    void updateSaying(String saying) {
        if (xSemaphoreTake(lvglMutex, 100 / portTICK_PERIOD_MS)) {
            lv_label_set_text(sayingLabel, saying.c_str());
            xSemaphoreGive(lvglMutex);
        }
    }

    void begin() {
        GFXDriver::init();
        lvglMutex = xSemaphoreCreateMutex();

        timeLabel = lv_label_create(lv_scr_act());
        lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -40);
        // lv_label_set_text(timeLabel, "11:45:14");
        lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_36,
                                   LV_PART_MAIN);

        dateLabel = lv_label_create(lv_scr_act());
        lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, -8);
        lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_18,
                                   LV_PART_MAIN);

        dayOfWeekLabel = lv_label_create(lv_scr_act());
        lv_obj_align(dayOfWeekLabel, LV_ALIGN_CENTER, 0, 16);
        lv_obj_set_style_text_font(dayOfWeekLabel, &lv_font_montserrat_18,
                                   LV_PART_MAIN);

        sayingLabel = lv_label_create(lv_scr_act());
        lv_obj_align(sayingLabel, LV_ALIGN_CENTER, 0, 40);

        networkBtn = lv_imagebutton_create(lv_scr_act());
        lv_obj_align(networkBtn, LV_IMAGE_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_size(networkBtn, 64, 64);
        lv_imagebutton_set_src(networkBtn, LV_IMAGEBUTTON_STATE_RELEASED,
                               nullptr, &wifi_connected, nullptr);

        lv_obj_add_event_cb(networkBtn, networkButtonHandler, LV_EVENT_CLICKED,
                            nullptr);

        xTaskCreate(updateDateTimeStringTask, "UpdateTimeStringTask", 4096,
                    nullptr, 1, nullptr);

        
    }
} // namespace Graphics
