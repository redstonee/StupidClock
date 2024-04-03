#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <vector>

#include "Network.hpp"
#include "config.h"

extern "C" {
LV_IMG_DECLARE(wifi_connected)
}

namespace GFXDriver {
static TFT_eSPI tft;

void display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  // tft.pushColors(&color_map->full, w * h, true);
  // tft.pushColors((uint16_t *)px_map, w * h, true);
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

  static uint32_t draw_buf[LCD_WIDTH * LCD_HEIGHT / 4];
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
inline void initDriver() { GFXDriver::init(); }

void drawTest() {
  auto label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Are you an idiot?");
  lv_obj_center(label);

  static uint16_t cnt = 0;
  static auto cntText = lv_label_create(lv_scr_act());
  lv_label_set_text_fmt(cntText, "You've been fucked %d times!", cnt);
  lv_obj_align(cntText, LV_ALIGN_BOTTOM_MID, 0, -20);

  auto btnHandler = [](lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
      lv_label_set_text_fmt(cntText, "You've been fucked %d times!", ++cnt);
    }
  };

  auto btn = lv_button_create(lv_scr_act());
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 30);
  lv_obj_set_size(btn, 80, 30);
  lv_obj_add_event_cb(btn, btnHandler, LV_EVENT_CLICKED, nullptr);

  auto btnText = lv_label_create(btn);
  lv_label_set_text(btnText, "Fuck Me");
  lv_obj_center(btnText);
}

void drawUI() {
  auto timeLabel = lv_label_create(lv_scr_act());
  lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -30);
  lv_label_set_text(timeLabel, "11:45:14");
  lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_36, LV_PART_MAIN);

  auto networkBtn = lv_imagebutton_create(lv_scr_act());
  lv_obj_align(networkBtn, LV_IMAGE_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_size(networkBtn, 64, 64);
  lv_imagebutton_set_src(networkBtn, LV_IMAGEBUTTON_STATE_RELEASED, nullptr,
                         &wifi_connected, nullptr);

  lv_obj_add_event_cb(
      networkBtn,
      [](lv_event_t *e) {
        xTaskCreate(Network::smartConfigTask, "SmartConfigTask", 4096, nullptr,
                    1, nullptr);
      },
      LV_EVENT_CLICKED, nullptr);
      
}
} // namespace Graphics