#pragma once

#include <Arduino.h>

#include "Config.h"

class LedPanel;
class FlowersNetworkManager;

// Effects engine: group cascades (full/top/bottom) and the 14-minute sunrise.
// Breathing and glitch are added in Block 6.
class EffectsManager {
 public:
  void begin(LedPanel* panels, uint8_t panelCount,
             FlowersNetworkManager* network);

  // Called from loop(): advances sunrise and the group cascade.
  void update();

  // Group commands from MQTT.
  void setFullLight(bool on);
  void setTopLight(bool on);
  void setBottomLight(bool on);

  // Sunrise commands from MQTT.
  bool startSunrise();
  void stopSunrise();

  // Any manual panel/brightness command cancels sunrise and an active cascade.
  void onManualOverride();

  bool sunriseActive() const { return _sunriseActive; }
  bool waveActive() const { return _wave.active; }

 private:
  struct Wave {
    bool active = false;
    uint8_t sequence[cfg::kPanelCount] = {};
    uint8_t count = 0;
    uint8_t index = 0;
    bool turnOn = true;
    unsigned long lastStepMs = 0;
  };

  void startWave(const uint8_t* sequence, uint8_t count, bool turnOn);
  void stepWave();
  void updateWave();
  void updateSunrise();

  LedPanel* _panels = nullptr;
  uint8_t _panelCount = 0;
  FlowersNetworkManager* _network = nullptr;

  bool _sunriseActive = false;
  unsigned long _sunriseStartMs = 0;
  bool _sunriseStarted[cfg::kPanelCount] = {};

  Wave _wave;
};
