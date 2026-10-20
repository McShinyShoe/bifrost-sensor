#pragma once

#include <Arduino.h>

#define W5100_CS_PIN 10
#define SD_CS_PIN 4

#define BIFROST_MAC {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}

#define BIFROST_FALLBACK_IP 10, 10, 0, 50
#define BIFROST_FALLBACK_DNS 1, 1, 1, 1
#define BIFROST_FALLBACK_GATEWAY 10, 10, 0, 1
#define BIFROST_FALLBACK_SUBNET 255, 255, 255, 0

#define DHCP_TIMEOUT_MS 15000UL

#define DHT_PIN 2
#define DHT_TYPE DHT11

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2
