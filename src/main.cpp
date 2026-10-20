// Entry point. Blocks 1-5 wire configuration, the five LED panels, the
// network layer and the effects engine (cascades + sunrise).
// Breathing/glitch are attached in Block 6.

#include <Arduino.h>

#include "Cie1931.h"
#include "Config.h"
#include "EffectsManager.h"
#include "LedPanel.h"
#include "NetworkManager.h"

LedPanel panels[cfg::kPanelCount];
FlowersNetworkManager network;
EffectsManager effects;

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

void printCieCheck() {
  Serial.printf("CIE1931: 0%%=%u, 50%%=%u, 100%%=%u\n",
                static_cast<unsigned>(cie1931::percentToDuty(0.0f)),
                static_cast<unsigned>(cie1931::percentToDuty(50.0f)),
                static_cast<unsigned>(cie1931::percentToDuty(100.0f)));
}

void initPanels() {
  for (uint8_t i = 0; i < cfg::kPanelCount; ++i) {
    if (!panels[i].begin(cfg::kPanelPins[i], static_cast<uint8_t>(i + 1))) {
      Serial.printf("PANEL %u: init failed\n", static_cast<unsigned>(i + 1));
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  // Guarantees that the USB-CDC console is attached before the first log line.
  delay(cfg::kBootUsbDelayMs);

  config.begin();
  printBootInfo();
  printCieCheck();
  initPanels();

  effects.begin(panels, cfg::kPanelCount, &network);
  network.setEffectsManager(&effects);
  network.begin(panels, cfg::kPanelCount);
}

void loop() {
  for (LedPanel& panel : panels) {
    panel.update();
  }
  effects.update();
  network.update();
  delay(1);
}
