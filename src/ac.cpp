#include <string.h>

#include "ac.h"
#include "config.h"

// IRremote only generates the carrier
#define DISABLE_CODE_FOR_RECEIVER
#include <IRremote.hpp>

namespace shiny {
    namespace ac {
        Remote remote(IR_IN_PIN);
        Unit unit(IR_OUT_PIN);

        bool Settings::valid() const {
            return (uint8_t)mode <= (uint8_t)Mode::Smart &&
            (uint8_t)fan <= (uint8_t)Fan::Speed5 &&
                   celsius >= kMinCelsius &&
                   celsius <= kMaxCelsius &&
                   sleep <= kMaxSleep &&
                   roomCelsius <= kMaxRoomCelsius &&
                   onTimerHours <= kMaxTimerHours &&
                   offTimerHours <= kMaxTimerHours;
        }

        void Signal::encode(uint8_t frame[kFrameBytes]) const {
            static const uint8_t kModeBits[5] = {2, 3, 4, 0, 1}; // Cool, Dry, Fan, Heat, Smart
            static const uint8_t kFanBits[6] = {0x00, 0x03, 0x43, 0x02, 0x41, 0x01};
            const uint8_t modeIndex = (uint8_t)mode <= 4 ? (uint8_t)mode : 0; // out of range: cool / auto
            const uint8_t fanBits = kFanBits[(uint8_t)fan <= 5 ? (uint8_t)fan : 0];
            uint8_t* f = frame;

            memset(f, 0, kFrameBytes);
            f[0] = 0x83;
            f[1] = 0x06;
            f[2] = (uint8_t)((fanBits & 0x03) | (sleep ? 0x08 : 0) | (powerToggle ? 0x04 : 0) | (swingToggle ? 0x80 : 0));
            f[3] = (uint8_t)(kModeBits[modeIndex] | ((celsius - 16) << 4));
            f[4] = mode == Mode::Smart ? 0x80 : 0x00;
            f[5] = super ? 0x90 : 0x00;
            f[6] = (uint8_t)(0x80 | clockHours | (displayToggle ? 0x20 : 0));
            f[7] = (uint8_t)(clockMinutes | (offTimerHours ? 0x80 : 0));
            f[8] = (uint8_t)(offTimerHours | (swingToggle ? 0x40 : 0));
            f[9] = onTimerHours ? 0x80 : 0x00;
            f[10] = onTimerHours;
            f[11] = ifeel ? 0x80 : 0x00;
            f[12] = ifeel ? roomCelsius : 0;
            f[14] = (uint8_t)((quiet ? 0x04 : 0) | (economy ? 0x20 : 0) | (sleep ? (sleep - 1) << 6 : 0));
            f[15] = (uint8_t)button;
            f[17] = fanBits & 0x40;
            f[18] = 0x08;
            f[13] = xorBytes(f + 2, 11);
            f[20] = xorBytes(f + 14, 6);
        }

        bool Signal::decode(const uint8_t frame[kFrameBytes]) {
            const uint8_t* f = frame;

            if (f[0] != 0x83 || f[1] != 0x06 || xorBytes(f + 2, 11) != f[13] || xorBytes(f + 14, 6) != f[20]) return false;

            static const Mode kModes[5] = {Mode::Heat, Mode::Smart, Mode::Cool, Mode::Dry, Mode::Fan};
            if ((f[3] & 7) > 4) return false;

            Signal s;
            s.mode = kModes[f[3] & 7];
            const bool fan2 = f[17] & 0x40;
            switch (f[2] & 3) {
                case 0: s.fan = Fan::Auto; break;
                case 1: s.fan = fan2 ? Fan::Speed4 : Fan::Speed5; break;
                case 2: s.fan = Fan::Speed3; break;
                default: s.fan = fan2 ? Fan::Speed2 : Fan::Speed1; break;
            }
            s.celsius = (uint8_t)((f[3] >> 4) + 16);
            s.sleep = (f[2] & 0x08) ? (uint8_t)((f[14] >> 6) + 1) : 0;
            s.super = (f[5] & 0x90) == 0x90;
            s.quiet = f[14] & 0x04;
            s.economy = f[14] & 0x20;
            s.ifeel = f[11] & 0x80;
            s.roomCelsius = s.ifeel ? f[12] : 0;
            s.onTimerHours = (f[9] & 0x80) ? (f[10] & 0x1F) : 0;
            s.offTimerHours = (f[7] & 0x80) ? (f[8] & 0x1F) : 0;
            s.button = (Button)f[15];
            s.powerToggle = f[2] & 0x04;
            s.swingToggle = f[2] & 0x80;
            s.displayToggle = f[6] & 0x20;
            s.clockHours = f[6] & 0x1F;
            s.clockMinutes = f[7] & 0x3F;

            if (!s.valid()) return false;
            *this = s;

            return true;
        }

        void Signal::print(Print& out) const {
            out.print(F("button "));
            out.print(buttonName(button));
            if (powerToggle) out.print(F(" (flips power)"));
            if (swingToggle) out.print(F(" (flips swing)"));
            if (displayToggle) out.print(F(" (flips display)"));
            out.print(F(": "));
            out.print(modeName(mode));
            out.print(F(", fan "));
            out.print(fanName(fan));
            out.print(F(", "));
            out.print(celsius);
            out.print(F(" C"));
            if (super) out.print(F(", super"));
            if (sleep) {
                out.print(F(", sleep "));
                out.print(sleep);
            }
            if (quiet) out.print(F(", quiet"));
            if (economy) out.print(F(", economy"));
            if (ifeel) {
                out.print(F(", ifeel "));
                out.print(roomCelsius);
                out.print(F(" C"));
            }
            if (onTimerHours) {
                out.print(F(", timer on "));
                out.print(onTimerHours);
                out.print(F(" h"));
            }
            if (offTimerHours) {
                out.print(F(", timer off "));
                out.print(offTimerHours);
                out.print(F(" h"));
            }
            out.print(F(", clock "));
            printTwoDigits(out, clockHours);
            out.print(':');
            printTwoDigits(out, clockMinutes);
            out.println();
        }

        void Signal::printFrame(Print& out) const {
            uint8_t f[kFrameBytes];
            encode(f);

            for (uint8_t i = 0; i < kFrameBytes; ++i) {
                if (i) out.print(' ');
                if (f[i] < 0x10) out.print('0');
                out.print(f[i], HEX);
            }
            out.println();
        }

        const __FlashStringHelper* Signal::modeName(Mode m) {
            switch (m) {
                case Mode::Cool: return F("cool");
                case Mode::Dry: return F("dry");
                case Mode::Fan: return F("fan");
                case Mode::Heat: return F("heat");
                case Mode::Smart: return F("smart");
            }

            return F("?");
        }

        const __FlashStringHelper* Signal::fanName(Fan f) {
            switch (f) {
                case Fan::Auto: return F("auto");
                case Fan::Speed1: return F("3 bars");
                case Fan::Speed2: return F("4 bars");
                case Fan::Speed3: return F("6 bars");
                case Fan::Speed4: return F("8 bars");
                case Fan::Speed5: return F("full");
            }

            return F("?");
        }

        const __FlashStringHelper* Signal::buttonName(Button b) {
            switch (b) {
                case Button::Display: return F("dimmer/clock");
                case Button::Power: return F("power");
                case Button::Temp: return F("temp");
                case Button::Sleep: return F("sleep");
                case Button::Super: return F("super");
                case Button::OnTimer: return F("timer on");
                case Button::Mode: return F("mode");
                case Button::Swing: return F("swing");
                case Button::Quiet: return F("quiet");
                case Button::Economy: return F("economy");
                case Button::IFeel: return F("ifeel");
                case Button::Fan: return F("fan");
                case Button::Smart: return F("smart");
                case Button::OffTimer: return F("timer off");
            }

            return F("unknown");
        }

        void Unit::begin() {
            pinMode(pin_, OUTPUT);
            digitalWrite(pin_, LOW);
        }

        bool Unit::sendPowerOn() {
            if (state_.power) { // thinks its on: refresh only, toggle would switch it off
                transmit(signal(Button::Temp));
                return true;
            }

            state_.power = true;
            clearOnPowerSwitch();
            transmit(signal(Button::Power));

            return true;
        }

        bool Unit::sendPowerOff() {
            if (!state_.power) return false; // thinks its off: toggle would switch it on

            state_.power = false;
            clearOnPowerSwitch();
            transmit(signal(Button::Power));

            return true;
        }

        void Unit::togglePower() {
            state_.power = !state_.power;
            clearOnPowerSwitch();
            transmit(signal(Button::Power));
        }

        bool Unit::setTemperature(uint8_t celsius) {
            if (celsius < Settings::kMinCelsius || celsius > Settings::kMaxCelsius) return false;

            state_.celsius = celsius;
            state_.super = false;
            return commit(Button::Temp);
        }

        bool Unit::setMode(Mode mode) {
            if ((uint8_t)mode > (uint8_t)Mode::Smart) return false;

            state_.mode = mode;
            state_.super = false;
            if (mode == Mode::Smart) state_.sleep = 0;

            return commit(mode == Mode::Smart ? Button::Smart : Button::Mode);
        }

        bool Unit::setFan(Fan fan) {
            if ((uint8_t)fan > (uint8_t)Fan::Speed5) return false;

            state_.fan = fan;
            state_.super = false;

            return commit(Button::Fan);
        }

        bool Unit::setSwing(bool on) {
            if (!state_.power || state_.swing == on) return false;

            state_.swing = on;
            transmit(signal(Button::Swing));

            return true;
        }

        bool Unit::toggleDisplay() {
            if (!state_.power) return false;

            transmit(signal(Button::Display));

            return true;
        }

        bool Unit::setSuper(bool on) {
            if (!state_.power) return false;

            state_.super = on;
            if (on) {
                if (state_.mode != Mode::Heat) state_.mode = Mode::Cool;
                state_.sleep = 0;
                state_.quiet = false;
                state_.economy = false;
            }
            transmit(signal(Button::Super));

            return true;
        }

        bool Unit::setSleep(uint8_t stars) {
            if (!state_.power || stars > Settings::kMaxSleep) return false;

            state_.sleep = stars;
            if (stars) {
                state_.super = false;
                state_.economy = false;
            }
            transmit(signal(Button::Sleep));

            return true;
        }

        bool Unit::setQuiet(bool on) {
            if (!state_.power) return false;

            state_.quiet = on;
            if (on) {
                state_.super = false;
                state_.economy = false;
            }
            transmit(signal(Button::Quiet));

            return true;
        }

        bool Unit::setEconomy(bool on) {
            if (!state_.power) return false;

            state_.economy = on;
            if (on) {
                state_.super = false;
                state_.quiet = false;
                state_.sleep = 0;
            }
            transmit(signal(Button::Economy));

            return true;
        }

        bool Unit::setIFeel(bool on, uint8_t roomCelsius) {
            if (!state_.power || (on && roomCelsius > Settings::kMaxRoomCelsius)) return false;

            state_.ifeel = on;
            state_.roomCelsius = on ? roomCelsius : 0;
            transmit(signal(Button::IFeel));

            return true;
        }

        bool Unit::setOnTimer(uint8_t hours) {
            if (hours > Settings::kMaxTimerHours) return false;

            state_.onTimerHours = hours;
            transmit(signal(Button::OnTimer));

            return true;
        }

        bool Unit::setOffTimer(uint8_t hours) {
            if (hours > Settings::kMaxTimerHours) return false;

            state_.offTimerHours = hours;
            transmit(signal(Button::OffTimer));

            return true;
        }

        bool Unit::sync(const Signal& signal, bool sendAfter) {
            if (!signal.valid()) return false;

            const Fan keepFan = state_.fan;
            const uint8_t keepCelsius = state_.celsius;
            static_cast<Settings&>(state_) = signal;
            const bool tempOverridden = signal.super || signal.mode == Mode::Smart;

            if (tempOverridden || signal.sleep || signal.quiet || signal.economy) state_.fan = keepFan;
            if (tempOverridden) state_.celsius = keepCelsius;
            if (signal.powerToggle) {
                state_.power = !state_.power;
                state_.swing = false;
            }
            if (signal.swingToggle) state_.swing = !state_.swing;
            if (sendAfter) transmit(signal);

            return true;
        }

        Signal Unit::signalFor(const State& state, Button button) {
            Signal s;
            static_cast<Settings&>(s) = state;
            s.button = button;
            s.powerToggle = button == Button::Power;
            s.swingToggle = button == Button::Swing;
            s.displayToggle = button == Button::Display;
            if (s.mode == Mode::Fan && s.fan == Fan::Auto) s.fan = Fan::Speed5; // never auto here
            if (s.super) {
                s.fan = Fan::Speed5;
                if (s.mode == Mode::Heat) {
                    s.celsius = Settings::kMaxCelsius;
                } else {
                    s.mode = Mode::Cool;
                    s.celsius = Settings::kMinCelsius;
                }
            } else if (s.mode == Mode::Smart) {
                s.fan = Fan::Auto; // smart picks its own fan and runs at 23 C
                s.celsius = 23;
            } else if (s.sleep || s.quiet || s.economy) {
                s.fan = Fan::Speed1;
            }

            return s;
        }

        // What the unit (and the real remote) drop whenever the power is switched.
        void Unit::clearOnPowerSwitch() {
            state_.swing = false;
            state_.super = false;
            state_.sleep = 0;
            state_.quiet = false;
            state_.economy = false;
            state_.ifeel = false;
            state_.roomCelsius = 0;
        }

        void Unit::transmit(const Signal& signal) {
            uint8_t f[Signal::kFrameBytes];
            signal.encode(f);
            detail::txBusy() = true;
            IrSender.setSendPin(pin_);
            IrSender.enableIROut(38);
            IrSender.mark(kHdrMark);
            IrSender.space(kHdrSpace);
            sendBytes(f, 6);
            IrSender.mark(kBitMark);
            IrSender.space(kSectionGap);
            sendBytes(f + 6, 8);
            IrSender.mark(kBitMark);
            IrSender.space(kSectionGap);
            sendBytes(f + 14, 7);
            IrSender.mark(kBitMark);
            noInterrupts();
            detail::txBusy() = false;
            detail::txGuard() = true;
            detail::txEndMicros() = micros();
            interrupts();
            last_ = signal;
            sent_ = true;
        }

        void Unit::sendBytes(const uint8_t* d, uint8_t n) {
            for (; n; --n, ++d) {
                uint8_t b = *d;
                for (uint8_t i = 0; i < 8; ++i, b >>= 1) {
                    IrSender.mark(kBitMark);
                    IrSender.space((b & 1) ? kOneSpace : kZeroSpace);
                }
            }
        }

        bool Remote::begin() {
            const int irq = digitalPinToInterrupt(pin_);
            if (irq == NOT_AN_INTERRUPT) return false;
            pinMode(pin_, INPUT_PULLUP);
            active() = this;
            attachInterrupt(irq, isr, CHANGE);

            return true;
        }

        void Remote::end() {
            detachInterrupt(digitalPinToInterrupt(pin_));

            if (active() == this) active() = nullptr;
        }

        bool Remote::poll() {
            noInterrupts();
            const bool complete = started_ && (ready_ || (uint32_t)(micros() - lastEdge_) >= kEndGapMicros);
            if (complete) ready_ = true; // the interrupt leaves buf_ alone from here on
            const uint16_t n = len_;
            const bool overflow = overflow_;
            interrupts();
            if (!complete) return false;

            uint8_t frame[Signal::kFrameBytes];
            Signal signal;
            const bool ok = !overflow && frameFromTimings(buf_, n, frame) && signal.decode(frame);
            noInterrupts();
            started_ = ready_ = overflow_ = false;
            len_ = 0;
            interrupts();
            if (!ok) return false;

            last_ = signal;
            if (callback_) callback_(signal);

            return true;
        }

        bool Remote::frameFromTimings(const uint16_t* d, uint16_t n, uint8_t frame[Signal::kFrameBytes]) {
            static const uint8_t kSectionEnd[3] = {6, 14, 21};
            if (n < 3 || d[0] < kHdrMarkMin || d[1] < kHdrSpaceMin) return false;

            memset(frame, 0, Signal::kFrameBytes);
            uint8_t pos = 0, bit = 0, section = 0;
            for (uint16_t i = 2; i + 1 < n; i += 2) { // pairs of bit mark + space
                const uint16_t space = d[i + 1];
                if (space >= kGapSpaceMin) { // end of a section
                    if (bit || pos != kSectionEnd[section] || ++section == 3) return false;
                    continue;
                }
                if (pos == Signal::kFrameBytes) return false;

                if (space >= kOneSpaceMin) frame[pos] |= (uint8_t)(1 << bit);
                if (++bit == 8) {
                    bit = 0;
                    ++pos;
                }
            }

            return section == 2 && pos == Signal::kFrameBytes && bit == 0;
        }

        Remote*& Remote::active() {
            static Remote* listening = nullptr;

            return listening;
        }

        void Remote::isr() {
            Remote* r = active();
            if (r) r->onEdge();
        }

        void Remote::onEdge() {
            const uint32_t now = micros();
            if (detail::txBusy() ||
                (detail::txGuard() && (uint32_t)(now - detail::txEndMicros()) < kEchoGuardMicros)) {
                if (!ready_) started_ = false;
                return;
            }
            detail::txGuard() = false;
            if (ready_) return;

            if (!started_) {
                if (digitalRead(pin_) == LOW) {
                    started_ = true;
                    overflow_ = false;
                    len_ = 0;
                    lastEdge_ = now;
                }
                return;
            }
            const uint32_t d = now - lastEdge_;
            lastEdge_ = now;
            if (d >= kEndGapMicros) {
                ready_ = true;
                return;
            }
            if (len_ < kMaxEntries) buf_[len_++] = (uint16_t)d;
            else overflow_ = true;
        }
    }
}
