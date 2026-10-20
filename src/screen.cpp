#include "config.h"
#include "screen.h"

namespace shiny {
    Screen screen(LiquidCrystal_I2C(LCD_ADDR, LCD_COLS, LCD_ROWS));

    void Screen::init() {
        Serial.println(F("[screen] starting lcd"));

        lcd.init();
        lcd.backlight();
        lcd.clear();
        lcd.setCursor(0, 0);;
        lcd.print("Hello World");

        Serial.println(F("[screen] ready"));
    }

    void Screen::setActiveLayout(size_t id) {
        if (id >= layoutCount) return;

        activeLayoutID = id;
        lcd.clear();
        layouts[id]->invalidate();
        layouts[id]->update(lcd);
    }

    void Screen::update() {
        if (layoutCount == 0) return;

        layouts[activeLayoutID]->update(lcd);
    }
}
