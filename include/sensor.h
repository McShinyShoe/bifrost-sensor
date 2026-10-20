#pragma once

#include <DHT.h>

namespace shiny {
    using SensorUpdateCallback = void (*)(float humidity, float temperature);

    extern DHT dht;

    void sensorInit();

    float humidity();
    float temperature();

    void pollSensor();
    void setSensorUpdateCallback(SensorUpdateCallback callback);
}
