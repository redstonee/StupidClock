#include <Arduino.h>
#include <WiFi.h>

#include "Graphics.hpp"
#include "Network.hpp"
#include "RTC.hpp"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Graphics::initDriver();
  // Graphics::drawTest();
  Graphics::drawUI();
}

void loop() {
  // put your main code here, to run repeatedly:
  lv_task_handler(); /* let the GUI do its work */
  delay(5);          /* let this time pass */
}
