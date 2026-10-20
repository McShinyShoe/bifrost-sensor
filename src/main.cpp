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
  delay(100);
  Serial.print("Humidity: ");
  Serial.println(shiny::dht.readHumidity());
  Serial.print("Temperature: ");
  Serial.println(shiny::dht.readTemperature());
}
