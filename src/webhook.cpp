#include <Ethernet.h>
#include <string.h>

#include "webhook.h"
#include "ac_queue.h"
#include "config.h"
#include "http.h"

namespace shiny {
    namespace webhook {
        static EthernetServer server(WEBHOOK_PORT);

        // one write so the W5100 sends it as one packet, print(F()) would go byte by byte
        static void respond(EthernetClient& client, PGM_P status, PGM_P headers) {
            char head[112];
            const int len = snprintf_P(head, sizeof(head),
                PSTR("HTTP/1.0 %S\r\n%SContent-Length: 0\r\nConnection: close\r\n\r\n"), status, headers);
            if (len > 0 && len < (int)sizeof(head)) client.write((const uint8_t*)head, (size_t)len);
        }

        void init() {
            server.begin();
            Serial.print(F("[webhook] listening on port "));
            Serial.println(WEBHOOK_PORT);
        }

        // POST /poll makes the sensor check the ac queue now instead of waiting for the next interval
        void poll() {
            EthernetClient client = server.available();
            if (!client) return;

            char line[48];
            const bool gotLine = http::readLine(client, line, sizeof(line));
            char* path = gotLine ? strchr(line, ' ') : nullptr;
            if (path) {
                *path++ = '\0'; // line is just the method now
                char* end = strchr(path, ' ');
                if (end) *end = '\0';
            }

            // nothing in the headers or body matters, just get them out of the way
            char header[48];
            while (gotLine && http::readLine(client, header, sizeof(header)) && header[0]) {}
            while (client.available()) client.read();

            if (!path) {
                respond(client, PSTR("400 Bad Request"), PSTR(""));
            } else if (strcmp_P(path, PSTR("/poll")) != 0) {
                respond(client, PSTR("404 Not Found"), PSTR(""));
            } else if (strcmp_P(line, PSTR("POST")) != 0) {
                respond(client, PSTR("405 Method Not Allowed"), PSTR("Allow: POST\r\n"));
            } else {
                respond(client, PSTR("202 Accepted"), PSTR(""));
                Serial.println(F("[webhook] queue poll requested"));
                ac::requestPoll();
            }
            client.stop();
        }
    }
}
