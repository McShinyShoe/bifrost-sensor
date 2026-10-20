#include <Arduino.h>
#include <DHT.h>
#include <Ethernet.h>

#include "net.h"
#include "sensor.h"

void handleSensorUpdate(float humidity, float temperature);

void setup() {
  Serial.begin(115200);
  shiny::sensorInit();
  shiny::networkInit();

  pinMode(LED_BUILTIN, OUTPUT);

  shiny::setSensorUpdateCallback(handleSensorUpdate);
}

void loop() {
  shiny::pollSensor();
}

void handleSensorUpdate(float humidity, float temperature) {
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Temperature: ");
    Serial.println(temperature);
}
