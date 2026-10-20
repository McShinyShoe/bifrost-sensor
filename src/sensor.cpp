#include <DHT.h>

#include "sensor.h"
#include "config.h"

namespace shiny {
    DHT dht(DHT_PIN, DHT_TYPE);
    static SensorUpdateCallback sensorUpdateCallback = nullptr;

    void sensorInit() {
        dht.begin();
    }

    float humidity() {
        return dht.readHumidity();
    }

    float temperature() {
        return dht.readTemperature();
    }

    void pollSensor() {
        static float lastHumidity;
        static float lastTemperature;
        float currentHumidity = humidity();
        float currentTemperature = temperature();

        if (isnan(currentHumidity) || isnan(currentTemperature)) return;

        if (sensorUpdateCallback && (lastHumidity != currentHumidity || lastTemperature != currentTemperature)) {
            sensorUpdateCallback(lastHumidity = currentHumidity, lastTemperature = currentTemperature);
        }
    }

    void setSensorUpdateCallback(SensorUpdateCallback callback) {
        sensorUpdateCallback = callback;
    }
}
