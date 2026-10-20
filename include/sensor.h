#pragma once

#include <DHT.h>

namespace shiny {
    extern DHT dht;

    void sensorInit();

    float humidity();
    float temprature();

    void checkSensorUpdate();
    void onSensorUpdate(float h, float t);
}
