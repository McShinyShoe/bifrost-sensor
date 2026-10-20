#include <DHT.h>

#include "sensor.h"
#include "config.h"

namespace shiny {
    DHT dht(DHT_PIN, DHT_TYPE);

    void sensorInit() {
        dht.begin();
    }
}
