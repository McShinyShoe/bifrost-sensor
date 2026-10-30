#include <Arduino.h>
#include <DHT.h>
#include <Ethernet.h>

#include "clock.h"
#include "layout.h"
#include "net.h"
#include "pins_arduino.h"
#include "screen.h"
#include "sensor.h"
#include "ac.h"

void handleSensorUpdate(float humidity, float temperature);
void handleIRIn(shiny::ac::Signal signal);

void setup() {
    // out of gnd and vcc pins :sadcat:
    pinMode(7, OUTPUT);
    digitalWrite(7, LOW);
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    shiny::clock::init();
    shiny::sensor::init();
    shiny::net::init();
    shiny::screen.init();
    shiny::layout::init();
    shiny::ac::unit.begin();
    shiny::ac::unit.syncPowerState(true); // the ac is already on
    shiny::ac::remote.begin();


    shiny::sensor::setUpdateCallback(handleSensorUpdate);
    shiny::ac::remote.onSignal(handleIRIn);
}

unsigned long lastRun = 0;
bool ledPinToggle = 0;

void loop() {
    shiny::sensor::poll();
    shiny::screen.update();
    shiny::ac::remote.poll();
    unsigned long mls = millis();
    if(mls - lastRun >= 1000) {
        shiny::ac::unit.setTemperature(24);
        lastRun = mls;
        digitalWrite(LED_BUILTIN, ledPinToggle = !ledPinToggle);
    }
}

void handleSensorUpdate(float humidity, float temperature) {
    Serial.print("[");
    Serial.print(shiny::clock::now().timestamp());
    Serial.println("]");
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Temperature: ");
    Serial.println(temperature);
}

void handleIRIn(shiny::ac::Signal signal) {
    signal.print(Serial);
    shiny::ac::unit.sync(signal, false);
}
