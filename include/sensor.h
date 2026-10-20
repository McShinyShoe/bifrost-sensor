#pragma once

#include <DHT.h>

namespace shiny {
    namespace sensor {
        using UpdateCallback = void (*)(float humidity, float temperature);

        extern DHT dht;

        void init();

        float humidity();
        float temperature();

        void poll();
        void setUpdateCallback(UpdateCallback callback);
    }
}
