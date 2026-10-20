#pragma once

#include <DHT.h>

namespace shiny {
    extern DHT dht;

    void sensorInit();

    float humidity();
    float temprature();
}
