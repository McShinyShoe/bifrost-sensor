#include <Arduino.h>
#include <DHT.h>
#include <Ethernet.h>

#include "net.h"
#include "sensor.h"

void setup() {
  Serial.begin(115200);
  shiny::sensorInit();
  shiny::networkInit();

  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
  shiny::checkSensorUpdate();
}
