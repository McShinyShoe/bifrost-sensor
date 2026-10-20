#include <DHT.h>

#include "sensor.h"
#include "config.h"

namespace shiny {
    namespace sensor {
        DHT dht(DHT_PIN, DHT_TYPE);
        static UpdateCallback updateCallback = nullptr;

        void init() {
            dht.begin();
        }

        float humidity() {
            return dht.readHumidity();
        }

        float temperature() {
            return dht.readTemperature();
        }

        void poll() {
            static float lastHumidity;
            static float lastTemperature;
            float currentHumidity = humidity();
            float currentTemperature = temperature();

            if (isnan(currentHumidity) || isnan(currentTemperature)) return;

            if (updateCallback && (lastHumidity != currentHumidity || lastTemperature != currentTemperature)) {
                updateCallback(lastHumidity = currentHumidity, lastTemperature = currentTemperature);
            }
        }

        void setUpdateCallback(UpdateCallback callback) {
            updateCallback = callback;
        }
    }
}
