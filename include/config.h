#pragma once

#include <Arduino.h>

// Display size
constexpr uint16_t LCD_WIDTH = 320;
constexpr uint16_t LCD_HEIGHT = 240;

// Pin definitions
constexpr uint8_t DS_DATA = 19;
constexpr uint8_t DS_CLK = 18;
constexpr uint8_t DS_RST = 21;
constexpr uint8_t BUZZER = 13;

constexpr auto oneSayingURL = "https://api.xygeng.cn/openapi/one";

#define SMART_CONFIG_TIMEOUT_MS 3e4