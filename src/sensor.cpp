#include <DHT.h>

#include "sensor.h"
#include "config.h"

namespace shiny {
    DHT dht(DHT_PIN, DHT_TYPE);

    void sensorInit() {
        dht.begin();
    }

    float humidity() {
        return dht.readHumidity();
    }

    float temprature() {
        return dht.readTemperature();
    }

    void checkSensorUpdate() {
        static float lastHumidity;
        static float lastTemprature;
        float currentHumidity = humidity();
        float currentTemprature = temprature();

        if (lastHumidity != currentHumidity || lastTemprature != currentTemprature) {
            onSensorUpdate(lastHumidity = currentHumidity, lastTemprature = currentTemprature);
        }
    }

    void onSensorUpdate(float h, float t) {
        Serial.print("Humidity: ");
        Serial.println(shiny::humidity());
        Serial.print("Temperature: ");
        Serial.println(shiny::temprature());
    }
}
