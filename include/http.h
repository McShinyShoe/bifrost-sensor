#pragma once

#include <Arduino.h>
#include <Ethernet.h>

namespace shiny {
    namespace http {
        enum : int { kErrConnect = -1, kErrTimeout = -2, kErrMalformed = -3, kErrBadBody = -4 };

        int timedRead(Client& client); // -1 on timeout or connection closed
        bool readLine(Client& client, char* buf, size_t len); // drops the \r\n, and whatever doesnt fit in buff

        class Request {
         public:
          Request(const __FlashStringHelper* method, const char* path);
          ~Request() { client_.stop(); }

          Request(const Request&) = delete;
          Request& operator=(const Request&) = delete;

          int status() const { return status_; } // http status, or one of the kErr stuff
          int read(); // next body byte

         private:
          EthernetClient client_;
          int status_ = kErrConnect;
          long remaining_ = -1;
        };
    }
}
