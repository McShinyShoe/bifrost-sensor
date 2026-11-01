#pragma once

#include <Arduino.h>

#include <stdint.h>

namespace shiny {
    namespace ac {

        enum class Mode : uint8_t {
            Cool, Dry, Fan, Heat, Smart
        };
        enum class Fan : uint8_t {
            Auto, Speed1, Speed2, Speed3, Speed4, Speed5,
            Low = Speed1, Medium = Speed3, High = Speed5
        };

        enum class Button : uint8_t {
          Display = 0x00,
          Power = 0x01, Temp = 0x02, Sleep = 0x03, Super = 0x04, OnTimer = 0x05, Mode = 0x06,
          Swing = 0x07, Quiet = 0x0B, Economy = 0x0C, IFeel = 0x0D, Fan = 0x11, Smart = 0x17,
          OffTimer = 0x1D
        };

        class Settings {
         public:
          enum : uint8_t { kMinCelsius = 16, kMaxCelsius = 30, kMaxSleep = 4, kMaxTimerHours = 24, kMaxRoomCelsius = 50 };

          Mode mode = Mode::Cool;
          Fan fan = Fan::Auto;
          uint8_t celsius = 24;
          uint8_t sleep = 0; // 0: off, 1..4 stars
          bool super = false;
          bool quiet = false;
          bool economy = false;
          bool ifeel = false;
          uint8_t roomCelsius = 0; // sent if ifeel is on
          uint8_t onTimerHours = 0; // 0: not set
          uint8_t offTimerHours = 0;

          bool valid() const;
        };

        class Signal : public Settings {
         public:
          enum : uint8_t { kFrameBytes = 21 };

          Button button = Button::Temp;
          bool powerToggle = false;
          bool swingToggle = false;
          bool displayToggle = false;
          uint8_t clockHours = 0;
          uint8_t clockMinutes = 0;

          bool valid() const { return Settings::valid() && clockHours < 24 && clockMinutes < 60; }

          void encode(uint8_t frame[kFrameBytes]) const;
          bool decode(const uint8_t frame[kFrameBytes]);

          void print(Print& out) const;
          void printFrame(Print& out) const;

          static const __FlashStringHelper* modeName(Mode m);
          static const __FlashStringHelper* fanName(Fan f);
          static const __FlashStringHelper* buttonName(Button b);

         private:
          static uint8_t xorBytes(const uint8_t* p, uint8_t n) {
            uint8_t x = 0;
            while (n--) x ^= *p++;
            return x;
          }

          static void printTwoDigits(Print& out, uint8_t v) {
            if (v < 10) out.print('0');
            out.print(v);
          }
        };

        class State : public Settings {
         public:
          bool power = false;
          bool swing = false;
        };

        namespace detail {
            inline volatile bool& txBusy() {
              static volatile bool busy = false;
              return busy;
            }
            inline volatile bool& txGuard() {
              static volatile bool guard = false;
              return guard;
            }
            inline volatile uint32_t& txEndMicros() {
              static volatile uint32_t t = 0;
              return t;
            }
        }

        class Unit {
         public:
          explicit Unit(uint8_t irOutPin) : pin_(irOutPin) {}

          void begin();

          bool sendPowerOn();
          bool sendPowerOff();
          void togglePower();

          bool setTemperature(uint8_t celsius);
          bool setMode(Mode mode);
          bool setFan(Fan fan);
          bool setSwing(bool on);
          bool toggleDisplay();
          bool setSuper(bool on);
          bool setSleep(uint8_t stars);
          bool setQuiet(bool on);
          bool setEconomy(bool on);
          bool setIFeel(bool on, uint8_t roomCelsius);
          bool setOnTimer(uint8_t hours);
          bool setOffTimer(uint8_t hours);

          void send() { transmit(signal(Button::Temp)); }
          bool sync(const Signal& signal, bool sendAfter = false);
          void syncPowerState(bool on) { state_.power = on; clearOnPowerSwitch(); }
          void syncSwingState(bool on) { state_.swing = on; }
          const State& state() const { return state_; }
          Signal signal(Button button = Button::Temp) const { return signalFor(state_, button); }
          const Signal& lastSent() const { return last_; }
          bool hasSent() const { return sent_; }
          static Signal signalFor(const State& state, Button button);

          enum : uint16_t { kHdrMark = 8950, kHdrSpace = 4484, kBitMark = 597, kOneSpace = 1649, kZeroSpace = 533, kSectionGap = 7920 };

         private:
          bool commit(Button button) {
            if (state_.power) transmit(signal(button));
            return true;
          }
          void clearOnPowerSwitch();
          void transmit(const Signal& signal);
          static void sendBytes(const uint8_t* d, uint8_t n);
          uint8_t pin_;
          State state_;
          Signal last_;
          bool sent_ = false;
        } extern unit;

        class Remote {
         public:
          typedef void (*Callback)(Signal signal);

          enum : uint16_t {
            kMaxEntries = 360,
            kEndGapMicros = 30000,
            kEchoGuardMicros = 5000,
            kHdrMarkMin = 6000,
            kHdrSpaceMin = 3500,
            kOneSpaceMin = 1100,
            kGapSpaceMin = 5000
          };

          explicit Remote(uint8_t irInPin) : pin_(irInPin) {}

          bool begin();
          void end();

          void onSignal(Callback callback) { callback_ = callback; }
          bool poll();
          const Signal& lastSignal() const { return last_; }
          static bool frameFromTimings(const uint16_t* d, uint16_t n, uint8_t frame[Signal::kFrameBytes]);

         private:
          static Remote*& active();

          static void isr();

          void onEdge();

          uint8_t pin_;
          Callback callback_ = nullptr;
          Signal last_;
          volatile bool started_ = false;
          volatile bool ready_ = false;
          volatile bool overflow_ = false;
          volatile uint16_t len_ = 0;
          volatile uint32_t lastEdge_ = 0;
          uint16_t buf_[kMaxEntries];
        } extern remote;
    }
}
