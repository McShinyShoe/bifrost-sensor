#include <Arduino.h>
#include <DHT.h>
#include <Ethernet.h>

#include "net.h"
#include "sensor.h"

void handleSensorUpdate(float humidity, float temperature);

void setup() {
  Serial.begin(115200);
  shiny::sensor::init();
  shiny::net::init();

  pinMode(LED_BUILTIN, OUTPUT);

  shiny::sensor::setUpdateCallback(handleSensorUpdate);
}

void loop() {
  shiny::sensor::poll();
}

void handleSensorUpdate(float humidity, float temperature) {
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Temperature: ");
    Serial.println(temperature);
}
