#include <string.h>

#include "ac_commands.h"
#include "ac.h"

namespace shiny {
    namespace ac {
        enum : uint8_t { kMaxWords = 3 }; // "timer on 5" is da longest one

        struct Word {
            const char* text;
            size_t len;
        };

        static bool isSpace(char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }

        static uint8_t split(const char* p, Word words[kMaxWords]) {
            uint8_t n = 0;
            for (;;) {
                while (isSpace(*p)) ++p;
                if (!*p) return n;
                if (n == kMaxWords) return n + 1;

                const char* start = p;
                while (*p && !isSpace(*p)) ++p;
                words[n].text = start;
                words[n].len = (size_t)(p - start);
                ++n;
            }
        }

        static bool is(const Word& w, PGM_P keyword) {
            return w.len == strlen_P(keyword) && strncasecmp_P(w.text, keyword, w.len) == 0;
        }

        static bool toNumber(const Word& w, uint8_t& out) {
            if (w.len == 0 || w.len > 3) return false;

            uint16_t v = 0;
            for (size_t i = 0; i < w.len; ++i) {
                const char c = w.text[i];
                if (c < '0' || c > '9') return false;
                v = (uint16_t)(v * 10 + (c - '0'));
            }
            if (v > 255) return false;
            out = (uint8_t)v;

            return true;
        }

        static bool toSwitch(const Word& w, bool& out) {
            if (is(w, PSTR("on"))) out = true;
            else if (is(w, PSTR("off"))) out = false;
            else return false;

            return true;
        }

        static bool toMode(const Word& w, Mode& out) {
            for (uint8_t i = 0; i <= (uint8_t)Mode::Smart; ++i) {
                if (is(w, reinterpret_cast<PGM_P>(Signal::modeName((Mode)i)))) {
                    out = (Mode)i;
                    return true;
                }
            }

            return false;
        }

        static bool toFan(const Word& w, Fan& out) {
            uint8_t speed;
            if (is(w, PSTR("auto"))) out = Fan::Auto;
            else if (is(w, PSTR("low"))) out = Fan::Low;
            else if (is(w, PSTR("medium"))) out = Fan::Medium;
            else if (is(w, PSTR("high"))) out = Fan::High;
            else if (toNumber(w, speed) && speed >= (uint8_t)Fan::Speed1 && speed <= (uint8_t)Fan::Speed5) out = (Fan)speed;
            else return false;

            return true;
        }

        static CommandResult outcome(bool accepted) {
            return accepted ? CommandResult::Ok : CommandResult::Rejected;
        }

        CommandResult parseCommand(const char* command) {
            if (!command) return CommandResult::Empty;

            Word w[kMaxWords];
            const uint8_t n = split(command, w);
            if (n == 0) return CommandResult::Empty;

            const Word& name = w[0];
            bool on;
            uint8_t value;
            Mode mode;
            Fan fan;

            if (is(name, PSTR("power"))) {
                if (n != 2 || !is(w[1], PSTR("toggle"))) return CommandResult::BadArgument;
                unit.togglePower();
                return CommandResult::Ok;
            }
            if (is(name, PSTR("temperature"))) {
                if (n != 2 || !toNumber(w[1], value) ||
                    value < Settings::kMinCelsius || value > Settings::kMaxCelsius) return CommandResult::BadArgument;
                return outcome(unit.setTemperature(value));
            }
            if (is(name, PSTR("mode"))) {
                if (n != 2 || !toMode(w[1], mode)) return CommandResult::BadArgument;
                return outcome(unit.setMode(mode));
            }
            if (is(name, PSTR("fan"))) {
                if (n != 2 || !toFan(w[1], fan)) return CommandResult::BadArgument;
                return outcome(unit.setFan(fan));
            }
            if (is(name, PSTR("swing"))) {
                if (n != 2 || !toSwitch(w[1], on)) return CommandResult::BadArgument;
                return outcome(unit.setSwing(on));
            }
            if (is(name, PSTR("display"))) {
                if (n != 1) return CommandResult::BadArgument;
                return outcome(unit.toggleDisplay());
            }
            if (is(name, PSTR("super"))) {
                if (n != 2 || !toSwitch(w[1], on)) return CommandResult::BadArgument;
                return outcome(unit.setSuper(on));
            }
            if (is(name, PSTR("sleep"))) {
                if (n != 2) return CommandResult::BadArgument;
                if (is(w[1], PSTR("off"))) value = 0;
                else if (!toNumber(w[1], value) || value > Settings::kMaxSleep) return CommandResult::BadArgument;
                return outcome(unit.setSleep(value));
            }
            if (is(name, PSTR("quiet"))) {
                if (n != 2 || !toSwitch(w[1], on)) return CommandResult::BadArgument;
                return outcome(unit.setQuiet(on));
            }
            if (is(name, PSTR("economy"))) {
                if (n != 2 || !toSwitch(w[1], on)) return CommandResult::BadArgument;
                return outcome(unit.setEconomy(on));
            }
            if (is(name, PSTR("ifeel"))) {
                if (n != 2) return CommandResult::BadArgument;
                if (is(w[1], PSTR("off"))) return outcome(unit.setIFeel(false, 0));
                if (!toNumber(w[1], value) || value > Settings::kMaxRoomCelsius) return CommandResult::BadArgument;
                return outcome(unit.setIFeel(true, value));
            }
            if (is(name, PSTR("timer"))) {
                if (n != 3 || !toSwitch(w[1], on) || !toNumber(w[2], value) ||
                    value > Settings::kMaxTimerHours) return CommandResult::BadArgument;
                return outcome(on ? unit.setOnTimer(value) : unit.setOffTimer(value));
            }
            if (is(name, PSTR("send"))) {
                if (n != 1) return CommandResult::BadArgument;
                unit.send();
                return CommandResult::Ok;
            }

            return CommandResult::UnknownCommand;
        }

        const __FlashStringHelper* commandResultName(CommandResult result) {
            switch (result) {
                case CommandResult::Ok: return F("ok");
                case CommandResult::Empty: return F("empty command");
                case CommandResult::UnknownCommand: return F("unknown command");
                case CommandResult::BadArgument: return F("bad argument");
                case CommandResult::Rejected: return F("rejected, the ac is off or already set");
            }

            return F("?");
        }
    }
}
