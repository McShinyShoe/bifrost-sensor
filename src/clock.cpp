#include <Wire.h>
#include <RTClib.h>

#include "clock.h"

namespace shiny {
    namespace clock {
        RTC_DS1307 rtc;

        void init() {
            Serial.println(F("[clock] starting rtc"));

            if (!rtc.begin()) {
                Serial.println(F("[clock] no rtc detected, check the wiring"));
                return;
            }

            if (!rtc.isrunning()) {
                Serial.println(F("[clock] rtc is not running, setting it to the build time"));
                adjust(DateTime(F(__DATE__), F(__TIME__)));
            }

            Serial.print(F("[clock] time "));
            Serial.println(now().timestamp());
        }

        DateTime now() {
            return rtc.now();
        }

        void adjust(const DateTime &time) {
            rtc.adjust(time);
        }
    }
}
