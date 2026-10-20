#pragma once

#include <Arduino.h>

#include "Config.h"

class LedPanel;
class FlowersNetworkManager;

// Effects engine: group cascades (full/top/bottom), the 14-minute sunrise,
// the periodic breathing wave and the glitch generator.
class EffectsManager {
 public:
  void begin(LedPanel* panels, uint8_t panelCount,
             FlowersNetworkManager* network);

  // Called from loop(): advances sunrise, cascades, breathing and glitch.
  void update();

  // Group commands from MQTT.
  void setFullLight(bool on);
  void setTopLight(bool on);
  void setBottomLight(bool on);

  // Sunrise commands from MQTT.
  bool startSunrise();
  void stopSunrise();

  // Manual trigger for the breathing wave (startWave topic). Resets the
  // automatic 8-10 minute timer on success.
  bool startBreathingWave();

  // Any manual panel/brightness command cancels sunrise, cascades and
  // transient effects (breathing/glitch).
  void onManualOverride();

  bool sunriseActive() const { return _sunriseActive; }
  bool waveActive() const { return _wave.active; }
  bool breathingActive() const { return _breathing.active; }
  bool glitchActive() const { return _glitch.active; }

 private:
  struct Wave {
    bool active = false;
    uint8_t sequence[cfg::kPanelCount] = {};
    uint8_t count = 0;
    uint8_t index = 0;
    bool turnOn = true;
    unsigned long lastStepMs = 0;
  };

  struct BreathingWave {
    bool active = false;
    uint8_t sequence[cfg::kPanelCount] = {};
    uint8_t count = 0;
    uint8_t index = 0;
    unsigned long lastStepMs = 0;
  };

  struct Glitch {
    bool active = false;
    uint8_t panelIndex = 0;
    unsigned long endMs = 0;
    unsigned long nextFlickerMs = 0;
    uint16_t originalDuty = 0;
  };

  void startWave(const uint8_t* sequence, uint8_t count, bool turnOn);
  void stepWave();
  void updateWave();
  void updateSunrise();

  void stepBreathingWave();
  void updateBreathingWave();
  void finishBreathingWave();
  void cancelBreathingWave();

  void updateGlitch();
  void cancelGlitch();

  void cancelTransientEffects();
  void resetBreathingSchedule();
  void scheduleNextGlitch();

  LedPanel* _panels = nullptr;
  uint8_t _panelCount = 0;
  FlowersNetworkManager* _network = nullptr;

  bool _sunriseActive = false;
  unsigned long _sunriseStartMs = 0;
  bool _sunriseStarted[cfg::kPanelCount] = {};

  Wave _wave;

  BreathingWave _breathing;
  unsigned long _breathingLastMs = 0;
  unsigned long _breathingIntervalMs = 0;

  Glitch _glitch;
  unsigned long _glitchLastMs = 0;
  unsigned long _glitchIntervalMs = 0;
};
