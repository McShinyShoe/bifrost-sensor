#include <Arduino.h>
#include <DHT.h>
#include <Ethernet.h>

#include "net.h"
#include "sensor.h"
#include "config.h"

void setup() {
  Serial.begin(115200);
  shiny::sensorInit();

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  Ethernet.init(W5100_CS_PIN);
  shiny::startNetwork();
}

void loop() {
  delay(100);
  Serial.print("Humidity: ");
  Serial.println(shiny::dht.readHumidity());
  Serial.print("Temperature: ");
  Serial.println(shiny::dht.readTemperature());
}
