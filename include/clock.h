#pragma once

#include <RTClib.h>

namespace shiny {
    namespace clock {
        extern RTC_DS1307 rtc;

        void init();

        DateTime now();
        void adjust(const DateTime &time);
    }
}
