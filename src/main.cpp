#include <Arduino.h>
#include <WiFi.h>

#include "Graphics.h"
#include "Network.h"
#include "RTC.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Graphics::initDriver();
  Graphics::begin();

  Network::init();
  
}



void loop() {
  // put your main code here, to run repeatedly:
  if(xSemaphoreTake(lvglMutex, portMAX_DELAY) == pdTRUE) {
    lv_task_handler();
    xSemaphoreGive(lvglMutex);
  }
  delay(5);         
}
