#include <Dns.h>
#include <Ethernet.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "config.h"

namespace shiny {
    namespace http {
        static IPAddress serverIP;
        static bool resolved = false;

        static bool resolve() {
            if (resolved) return true;

            DNSClient dns;
            dns.begin(Ethernet.dnsServerIP());
            resolved = dns.getHostByName(BIFROST_HOST, serverIP, HTTP_TIMEOUT_MS) == 1;

            return resolved;
        }

        Request::Request(const __FlashStringHelper* method, const char* path) {
            if (!resolve()) return;

            client_.setConnectionTimeout(HTTP_TIMEOUT_MS);
            if (!client_.connect(serverIP, BIFROST_PORT)) {
                resolved = false; // maybe the ip changed? look it up again next time
                return;
            }

            char head[160];
            const int len = snprintf_P(head, sizeof(head),
                PSTR("%S %s HTTP/1.0\r\nHost: " BIFROST_HOST ":%u\r\nUser-Agent: bifrost-sensor\r\n\r\n"),
                (PGM_P)method, path, (unsigned)BIFROST_PORT);
            if (len < 0 || len >= (int)sizeof(head)) {
                status_ = kErrMalformed;
                return;
            }
            client_.write((const uint8_t*)head, (size_t)len);

            char line[48];
            if (!readLine(client_, line, sizeof(line))) {
                status_ = kErrTimeout;
                return;
            }
            if (strncmp_P(line, PSTR("HTTP/1."), 7) != 0 || strlen(line) < 12) {
                status_ = kErrMalformed;
                return;
            }
            status_ = atoi(line + 9);

            while (readLine(client_, line, sizeof(line)) && line[0]) {
                if (strncasecmp_P(line, PSTR("content-length:"), 15) == 0) remaining_ = atol(line + 15);
            }
        }

        int Request::read() {
            if (status_ < 0 || remaining_ == 0) return -1;

            const int c = timedRead(client_);
            if (c >= 0 && remaining_ > 0) --remaining_;

            return c;
        }

        int timedRead(Client& client) {
            const unsigned long start = millis();
            while (millis() - start < HTTP_TIMEOUT_MS) {
                const int c = client.read();
                if (c >= 0) return c;
                if (!client.connected()) return -1;
            }

            return -1;
        }

        bool readLine(Client& client, char* buf, size_t len) {
            size_t used = 0;
            for (;;) {
                const int c = timedRead(client);
                if (c < 0) {
                    buf[used] = '\0';
                    return false;
                }
                if (c == '\n') {
                    if (used && buf[used - 1] == '\r') --used;
                    buf[used] = '\0';
                    return true;
                }
                if (used + 1 < len) buf[used++] = (char)c;
            }
        }
    }
}
