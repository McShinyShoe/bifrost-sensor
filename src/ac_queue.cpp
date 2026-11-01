#include <string.h>

#include "ac_queue.h"
#include "ac_commands.h"
#include "config.h"
#include "http.h"

namespace shiny {
    namespace ac {
        enum : uint8_t { kBatchSize = 8, kCommandMax = 32 };

        struct Entry {
            uint32_t id;
            uint64_t timestamp;
            char command[kCommandMax];
            bool tooLong;
        };

        static bool before(const Entry& a, const Entry& b) {
            return a.timestamp != b.timestamp ? a.timestamp < b.timestamp : a.id < b.id;
        }

        struct Batch {
            Entry entries[kBatchSize];
            uint8_t count = 0;
            bool more = false;

            void insert(const Entry& e) {
                uint8_t i = count;
                if (count == kBatchSize) {
                    more = true;
                    if (!before(e, entries[kBatchSize - 1])) return;
                    --i; // drop the latest
                } else {
                    ++count;
                }
                for (; i && before(e, entries[i - 1]); --i) entries[i] = entries[i - 1];
                entries[i] = e;
            }
        };

        class QueueParser {
         public:
          explicit QueueParser(http::Request& in) : in_(in) { next(); }

          bool parse(Batch& batch) {
            if (!take('[')) return false;
            if (take(']')) return true;

            do {
              Entry e;
              if (!entry(e)) return false;
              batch.insert(e);
            } while (take(','));

            return take(']');
          }

         private:
          void next() { c_ = in_.read(); }

          void skipSpace() {
            while (c_ == ' ' || c_ == '\t' || c_ == '\r' || c_ == '\n') next();
          }

          bool take(char c) {
            skipSpace();
            if (c_ != c) return false;
            next();
            return true;
          }

          bool entry(Entry& e);
          bool string(char* out, uint8_t size, bool& fits);
          bool number(uint64_t& out);
          bool skipValue();

          http::Request& in_;
          int c_;
        };

        bool QueueParser::entry(Entry& e) {
            bool hasId = false, hasCommand = false;
            e.timestamp = 0;

            if (!take('{')) return false;
            do {
                char key[12];
                bool fits;
                if (!string(key, sizeof(key), fits) || !take(':')) return false;
                if (!fits) key[0] = '\0';

                if (strcmp_P(key, PSTR("id")) == 0) {
                    uint64_t id;
                    if (!number(id) || id > 0xFFFFFFFFUL) return false;
                    e.id = (uint32_t)id;
                    hasId = true;
                } else if (strcmp_P(key, PSTR("command")) == 0) {
                    if (!string(e.command, sizeof(e.command), fits)) return false;
                    e.tooLong = !fits;
                    hasCommand = true;
                } else if (strcmp_P(key, PSTR("timestamp")) == 0) {
                    if (!number(e.timestamp)) return false;
                } else if (!skipValue()) {
                    return false;
                }
            } while (take(','));

            return take('}') && hasId && hasCommand;
        }

        bool QueueParser::string(char* out, uint8_t size, bool& fits) {
            if (!take('"')) return false;

            uint8_t len = 0;
            fits = true;
            while (c_ != '"') {
                if (c_ == '\\') next();
                if (c_ < 0) return false;

                if (len + 1 < size) out[len++] = (char)c_;
                else fits = false;
                next();
            }
            next();
            out[len] = '\0';

            return true;
        }

        bool QueueParser::number(uint64_t& out) {
            skipSpace();
            if (c_ < '0' || c_ > '9') return false;

            uint64_t v = 0;
            for (uint8_t digits = 0; c_ >= '0' && c_ <= '9'; next()) {
                if (++digits > 19) return false; // 20+ digits might not fit >.<
                v = v * 10 + (uint8_t)(c_ - '0');
            }
            out = v;

            return true;
        }

        bool QueueParser::skipValue() {
            uint8_t depth = 0;
            skipSpace();
            while (c_ >= 0) {
                if (c_ == '"') {
                    char none[1];
                    bool fits;
                    if (!string(none, sizeof(none), fits)) return false;
                    continue;
                }
                if (!depth && (c_ == ',' || c_ == '}' || c_ == ']')) return true;

                if (c_ == '{' || c_ == '[') ++depth;
                else if (c_ == '}' || c_ == ']') --depth;
                next();
            }

            return false;
        }

        static bool succeeded(int status) {
            static int lastFailure = 0;
            if (status == 200) {
                lastFailure = 0;
                return true;
            }
            if (status != lastFailure) {
                lastFailure = status;
                Serial.print(F("[ac] queue request failed with status "));
                Serial.println(status);
            }

            return false;
        }

        static bool fetch(Batch& batch) {
            http::Request request(F("GET"), "/api/ac/command");
            if (!succeeded(request.status())) return false;

            QueueParser parser(request);
            return succeeded(parser.parse(batch) ? 200 : http::kErrBadBody);
        }

        static bool remove(uint32_t id) {
            char path[32];
            snprintf_P(path, sizeof(path), PSTR("/api/ac/command/%lu"), (unsigned long)id);

            http::Request request(F("DELETE"), path);
            return request.status() == 404 || succeeded(request.status()); // 404, aint no more
        }

        static void run(const Entry& e) {
            Serial.print(F("[ac] #"));
            Serial.print(e.id);
            Serial.print(' ');
            Serial.print(e.command);
            Serial.print(F(": "));
            Serial.println(e.tooLong ? F("command too long") : commandResultName(parseCommand(e.command)));
        }

        static unsigned long lastPoll = 0;
        static uint32_t undeleted = 0; // ran but DELETE failed, id only start from 1, 0 is none

        void pollQueue() {
            const unsigned long now = millis();
            if (now - lastPoll < AC_POLL_INTERVAL_MS) return;
            lastPoll = now;

            // nothing new runs until the server knows the last one is done, or it would just rerun it
            if (undeleted) {
                if (!remove(undeleted)) return;
                undeleted = 0;
            }

            Batch batch;
            if (!fetch(batch)) return;

            // bad and rejected commands get deleted too, otherwise they sit in the queue forever
            for (uint8_t i = 0; i < batch.count; ++i) {
                run(batch.entries[i]);
                if (!remove(batch.entries[i].id)) {
                    undeleted = batch.entries[i].id;
                    return;
                }
            }

            if (batch.more) lastPoll = now - AC_POLL_INTERVAL_MS;
        }
    }
}
