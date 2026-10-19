// Entry point. Block 1 only wires the configuration module; light, effects
// and network modules are attached in the following blocks.

#include <Arduino.h>

#include "Config.h"

namespace {

void printBootInfo() {
  Serial.println();
  Serial.println(F("=== flowers-esp32 (ESP32-C3 SuperMini) ==="));
  Serial.printf("CONFIG: fade=%.2f s, interval=%.2f s, NVS=%s\n",
                config.fadeTimeSec(), config.waveIntervalSec(),
                config.ready() ? "ok" : "fallback");
  Serial.print(F("PANELS (bottom->top): "));
  for (uint8_t i = 0; i < cfg::kPanelCount; ++i) {
    Serial.printf("%u%s", static_cast<unsigned>(cfg::kPanelPins[i]),
                  (i + 1 < cfg::kPanelCount) ? ", " : "\n");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  // Guarantees that the USB-CDC console is attached before the first log line.
  delay(cfg::kBootUsbDelayMs);

  config.begin();
  printBootInfo();
}

void loop() {
  // Module dispatching is added in the next blocks.
  delay(1000);
}
