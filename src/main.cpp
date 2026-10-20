#include <Arduino.h>
#include <Ethernet.h>

#include "net.h"
#include "config.h"

void blink(int ms);

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  Ethernet.init(W5100_CS_PIN);
  shiny::startNetwork();
}

void loop() {
  blink(500);
  Serial.println("alive");
}

void blink(int ms) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(ms);
  digitalWrite(LED_BUILTIN, LOW);
  delay(ms);
}
