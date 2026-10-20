#pragma once

#include <Arduino.h>

#include "Config.h"

// One shelf: an independent 12-bit LEDC channel with its own target
// brightness, fade state machine, breathing animation and raw-PWM hook
// (used by the glitch effect).
//
// Perceptual brightness is stored in percent and always converted through
// CIE1931 before it reaches the LEDC peripheral.
class LedPanel {
 public:
  LedPanel() = default;

  // Attaches the pin to LEDC (4 kHz, 12 bit) and drives it low.
  bool begin(uint8_t pin, uint8_t id);

  // Advances the fade/breathing state machine. Must be called from loop().
  void update();

  // Turns the shelf on/off with a smooth fade. The panel keeps its own
  // brightness percent while off, so the next ON restores it.
  void setOn(bool on, float durationSec = -1.0f);

  // Sets the individual brightness (1..100 %). When the panel is on it
  // fades to the new value; when off it only remembers it.
  void setBrightnessPercent(float percent, float durationSec = -1.0f);

  // Fades to 0 % and immediately back to the panel's own brightness.
  // Returns false if the panel is off or already breathing.
  bool startBreathing(float durationSec = 2.0f);

  // Cancels an active breathing animation without changing the on/off state.
  void cancelBreathing();

  // Direct duty cycle write, bypassing the fade state machine. Used by the
  // glitch effect and its exact restore.
  void setRawPwm(uint16_t duty);

  bool isOn() const { return _isOn; }
  bool isFading() const { return _fading; }
  bool isBreathing() const { return _breathing; }
  bool isLit() const { return _isOn && _currentDuty > 0; }

  uint8_t id() const { return _id; }
  uint8_t pin() const { return _pin; }
  float brightnessPercent() const { return _panelBrightnessPercent; }
  float currentPercent() const { return _currentPercent; }
  uint16_t currentDuty() const { return _currentDuty; }

  // One-shot event flags for the network layer. They are set when a command
  // changes the logical state/brightness and cleared by the consumer.
  bool consumeStateChanged();
  bool consumeBrightnessChanged();

 private:
  void fadeTo(float targetPercent, float durationSec);
  void applyPercent(float percent);
  void writeDuty(uint16_t duty);

  uint8_t _pin = 0;
  uint8_t _id = 0;
  uint8_t _channel = 0;
  bool _initialized = false;

  bool _isOn = false;
  float _panelBrightnessPercent =
      static_cast<float>(cfg::kDefaultBrightnessPercent);

  float _currentPercent = 0.0f;
  uint16_t _currentDuty = 0;

  bool _fading = false;
  float _fadeStartPercent = 0.0f;
  float _fadeTargetPercent = 0.0f;
  uint16_t _fadeStep = 0;
  int _stepDelayMs = 10;
  unsigned long _lastStepMs = 0;

  bool _breathing = false;
  bool _breathPhaseDown = false;
  float _breathDurationSec = 2.0f;

  bool _stateChanged = false;
  bool _brightnessChanged = false;
};
