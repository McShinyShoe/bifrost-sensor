#include <Ethernet.h>

#include "config.h"

namespace shiny {

    static uint8_t MAC[] = BIFROST_MAC;

    static void startNetwork() {
      Serial.println(F("[net] requesting a DHCP lease"));

      if (Ethernet.begin(MAC, DHCP_TIMEOUT_MS) != 0) {
        Serial.print(F("[net] address "));
        Serial.println(Ethernet.localIP());
        return;
      }

      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println(F("[net] no ethernet shield detected"));
      } else if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println(F("[net] link is down, check the cable"));
      }

      Serial.println(F("[net] DHCP failed, falling back to a static address"));
      Ethernet.begin(MAC, IPAddress(BIFROST_FALLBACK_IP),
                     IPAddress(BIFROST_FALLBACK_DNS),
                     IPAddress(BIFROST_FALLBACK_GATEWAY),
                     IPAddress(BIFROST_FALLBACK_SUBNET));

      Serial.print(F("[net] address "));
      Serial.println(Ethernet.localIP());
    }

}
