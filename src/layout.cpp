#include "layout.h"
#include "screen.h"
#include "sensor.h"

using Layout = shiny::Screen::ScreenLayout;

Layout::TextComponent temperatureLabel(0, 0, "Temp:");
Layout::ValueComponent<float> temperatureValue(6, 0, shiny::sensor::temperature, 6);
Layout::TextComponent humidityLabel(0, 1, "Hum:");
Layout::ValueComponent<float> humidityValue(6, 1, shiny::sensor::humidity, 6);

Layout::ScreenComponent *mainComponents[] = {
  &temperatureLabel, &temperatureValue,
  &humidityLabel, &humidityValue,
};
Layout mainLayout(mainComponents);

Layout *layouts[] = { &mainLayout };

namespace shiny {
    namespace layout {
        void init() {
            screen.setLayouts(layouts);
        }
    }
}
