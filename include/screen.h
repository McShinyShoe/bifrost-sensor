#pragma once

#include  <Arduino.h>
#include  <Wire.h>
#include  <LiquidCrystal_I2C.h>

namespace shiny {
    class Screen {
    public:
        class ScreenLayout {
        public:
            class ScreenComponent {
            public:
                ScreenComponent(uint8_t x, uint8_t y) : x(x), y(y) {}
                virtual void update(LiquidCrystal_I2C &lcd) = 0;

                void invalidate() { drawn = false; }

            protected:
                uint8_t x, y;
                bool drawn = false;
            };

            class TextComponent : public ScreenComponent {
            public:
                TextComponent(uint8_t x, uint8_t y, const char *text)
                    : ScreenComponent(x, y), text(text) {}

                void update(LiquidCrystal_I2C &lcd) override {
                    if (drawn) return;

                    drawn = true;
                    lcd.setCursor(x, y);
                    lcd.print(text);
                }

            private:
                const char *text;
            };

            template <typename T>
            class ValueComponent : public ScreenComponent {
            public:
                using FetchDataCallback = T (*)();

                ValueComponent(uint8_t x, uint8_t y, FetchDataCallback callback, uint8_t width = 0)
                    : ScreenComponent(x, y), callback(callback), width(width) {}

                void update(LiquidCrystal_I2C &lcd) override {
                    T currentData = callback();
                    if (drawn && currentData == lastData) return;

                    lastData = currentData;
                    drawn = true;
                    lcd.setCursor(x, y);

                    size_t written = lcd.print(currentData);
                    while (written++ < width) lcd.write(' ');
                }

            private:
                FetchDataCallback callback;
                uint8_t width;
                T lastData{};
            };

            template <size_t N>
            ScreenLayout(ScreenComponent *(&components)[N]) : components(components), count(N) {}

            void update(LiquidCrystal_I2C &lcd) {
                for (size_t i = 0; i < count; i++) components[i]->update(lcd);
            }

            void invalidate() {
                for (size_t i = 0; i < count; i++) components[i]->invalidate();
            }

        private:
            ScreenComponent **components;
            size_t count;
        };

        LiquidCrystal_I2C lcd;
        ScreenLayout **layouts = nullptr;
        size_t layoutCount = 0;
        size_t activeLayoutID = 0;

        Screen() = delete;
        Screen(LiquidCrystal_I2C lcd) : lcd(lcd) {};

        void init();

        template <size_t N>
        void setLayouts(ScreenLayout *(&layouts)[N]) {
            this->layouts = layouts;
            layoutCount = N;
            setActiveLayout(0);
        }

        void setActiveLayout(size_t id);
        void update();
    } extern screen;
}
