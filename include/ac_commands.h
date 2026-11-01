#pragma once

#include <Arduino.h>

#include <stdint.h>

namespace shiny {
    namespace ac {

        enum class CommandResult : uint8_t {
            Ok, Empty, UnknownCommand, BadArgument, Rejected
        };

        //   power toggle
        //   temperature <16-30>
        //   mode cool|dry|fan|heat|smart
        //   fan auto|low|medium|high|<1-5>
        //   swing on|off
        //   display
        //   super on|off
        //   sleep off|<0-4>
        //   quiet on|off
        //   economy on|off
        //   ifeel off|<0-50>
        //   timer on|off <0-24>
        //   send
        CommandResult parseCommand(const char* command);
        inline CommandResult parseCommand(const String& command) { return parseCommand(command.c_str()); }

        const __FlashStringHelper* commandResultName(CommandResult result);
    }
}
