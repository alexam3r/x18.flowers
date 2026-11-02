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

  // Sunset: every lit panel fades from its current perceived brightness to 0
  // over two hours. OFF stops the mode and turns the whole shelf off.
  bool startSunset();
  void stopSunset();

  // Manual trigger for the breathing wave (startWave topic). Resets the
  // automatic 8-10 minute timer on success.
  bool startBreathingWave();

  // Any manual panel/brightness command cancels sunrise, cascades and
  // transient effects (breathing/glitch). The full version also cancels the
  // sunset; the per-panel version cancels it only for that panel.
  void onManualOverride();
  void onManualOverrideForPanel(uint8_t panelIndex);

  bool sunriseActive() const { return _sunriseActive; }
  bool sunsetActive() const { return _sunsetActive; }
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
  void updateSunset();
  void finishSunset();
  void cancelSunset();
  void cancelSunsetPanel(uint8_t panelIndex);
  bool anySunsetPanelActive() const;
  void turnAllPanelsOffImmediately();

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

  bool _sunsetActive = false;
  unsigned long _sunsetStartMs = 0;
  unsigned long _sunsetLastStepMs = 0;
  float _sunsetStartPercent[cfg::kPanelCount] = {};
  bool _sunsetPanelActive[cfg::kPanelCount] = {};

  Wave _wave;

  BreathingWave _breathing;
  unsigned long _breathingLastMs = 0;
  unsigned long _breathingIntervalMs = 0;

  Glitch _glitch;
  unsigned long _glitchLastMs = 0;
  unsigned long _glitchIntervalMs = 0;
};
