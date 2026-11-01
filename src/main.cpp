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
#include "ac_commands.h"
#include "config.h"

void handleSensorUpdate(float humidity, float temperature);
void handleIRIn(shiny::ac::Signal signal);
void handleSerial();

void setup() {
    // out of gnd and vcc pins :sadcat:
    pinMode(DHT_GND_PIN, OUTPUT);
    digitalWrite(DHT_GND_PIN, LOW);
    pinMode(DHT_VCC_PIN, OUTPUT);
    digitalWrite(DHT_VCC_PIN, HIGH);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    shiny::clock::init();
    shiny::sensor::init();
    shiny::net::init();
    shiny::screen.init();
    shiny::layout::init();
    shiny::ac::unit.begin();
    shiny::ac::unit.syncPowerState(true); // the ac is already on
    if (!shiny::ac::remote.begin()) {
        Serial.println(F("[ac] ir receiver pin has no interrupt, use 2, 3, 18, 19, 20 or 21"));
    }

    shiny::sensor::setUpdateCallback(handleSensorUpdate);
    shiny::ac::remote.onSignal(handleIRIn);
}

unsigned long lastRun = 0;

void loop() {
    shiny::sensor::poll();
    shiny::screen.update();
    shiny::ac::remote.poll();
    handleSerial();
    unsigned long mls = millis();
    if(mls - lastRun >= 1000) {
        lastRun = mls;
        digitalWrite(LED_BUILTIN, HIGH);
    }
    if (mls - lastRun >= 50) {
        digitalWrite(LED_BUILTIN, LOW);

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

char serialLine[48];
uint8_t serialLen = 0;
bool serialOverflow = false;

// Collects a line without blocking (readStringUntil would stall the loop for up to 1s).
void handleSerial() {
    while (Serial.available()) {
        const char c = (char)Serial.read();
        if (c != '\n' && c != '\r') {
            if (serialLen < sizeof(serialLine) - 1) serialLine[serialLen++] = c;
            else serialOverflow = true;
            continue;
        }
        if (!serialLen && !serialOverflow) continue; // the \n after \r, or an empty line

        serialLine[serialLen] = '\0';
        if (serialOverflow) {
            Serial.println(F("[ac] command too long"));
        } else {
            Serial.print(F("[ac] "));
            Serial.print(serialLine);
            Serial.print(F(": "));
            Serial.println(shiny::ac::commandResultName(shiny::ac::parseCommand(serialLine)));
        }
        serialLen = 0;
        serialOverflow = false;
    }
}
