#include "LedPanel.h"

#include <cmath>

#include "Cie1931.h"

static_assert(cie1931::kMaxDuty == cfg::kPwmMaxDuty,
              "CIE1931 max duty must match the 12-bit PWM range");

namespace {
constexpr float kMinFadeSec = 0.1f;
constexpr float kMinBreathSec = 0.1f;
}  // namespace

bool LedPanel::begin(uint8_t pin, uint8_t id) {
  if (_initialized) return true;

  _pin = pin;
  _id = id;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  const bool attached =
      ledcAttach(_pin, cfg::kPwmFreqHz, cfg::kPwmResolutionBits);
#else
  _channel = (id > 0) ? static_cast<uint8_t>(id - 1) : 0;
  ledcSetup(_channel, cfg::kPwmFreqHz, cfg::kPwmResolutionBits);
  ledcAttachPin(_pin, _channel);
  const bool attached = true;
#endif

  if (!attached) {
    Serial.printf("PANEL %u: LEDC attach failed on GPIO %u\n",
                  static_cast<unsigned>(_id), static_cast<unsigned>(_pin));
    return false;
  }

  _isOn = false;
  _fading = false;
  _breathing = false;
  _currentPercent = 0.0f;
  _currentDuty = 0;
  _initialized = true;
  writeDuty(0);
  return true;
}

void LedPanel::setOn(bool on, float durationSec) {
  if (_breathing) cancelBreathing();

  if (_isOn != on) {
    _isOn = on;
    _stateChanged = true;
  }

  fadeTo(on ? _panelBrightnessPercent : 0.0f, durationSec);
}

void LedPanel::setBrightnessPercent(float percent, float durationSec) {
  float requested = percent;
  if (!(requested >= static_cast<float>(cfg::kMinBrightnessPercent))) {
    requested = static_cast<float>(cfg::kMinBrightnessPercent);
  }
  if (requested > static_cast<float>(cfg::kMaxBrightnessPercent)) {
    requested = static_cast<float>(cfg::kMaxBrightnessPercent);
  }

  if (fabsf(requested - _panelBrightnessPercent) > 0.001f) {
    _panelBrightnessPercent = requested;
    _brightnessChanged = true;
  }

  if (!_isOn) return;

  if (_breathing) cancelBreathing();
  fadeTo(_panelBrightnessPercent, durationSec);
}

bool LedPanel::startBreathing(float durationSec) {
  if (!_initialized || !_isOn || _breathing) return false;

  _breathing = true;
  _breathPhaseDown = true;
  _breathDurationSec = (durationSec < kMinBreathSec) ? kMinBreathSec : durationSec;

  fadeTo(0.0f, _breathDurationSec);
  return true;
}

void LedPanel::cancelBreathing() {
  if (!_breathing) return;
  _breathing = false;
  _breathPhaseDown = false;
  fadeTo(_isOn ? _panelBrightnessPercent : 0.0f, config.fadeTimeSec());
}

void LedPanel::setRawPwm(uint16_t duty) {
  if (!_initialized) return;
  if (duty > cie1931::kMaxDuty) duty = cie1931::kMaxDuty;
  writeDuty(duty);
}

void LedPanel::update() {
  if (!_initialized || !_fading) return;

  const unsigned long now = millis();
  const unsigned long elapsed = now - _lastStepMs;
  if (elapsed < static_cast<unsigned long>(_stepDelayMs)) return;

  // Catch up if the main loop was blocked (MQTT, effects, ...), while still
  // guaranteeing exactly kFadeSteps interpolation points.
  unsigned long steps = elapsed / static_cast<unsigned long>(_stepDelayMs);
  const uint16_t remaining =
      static_cast<uint16_t>(cfg::kFadeSteps - _fadeStep);
  if (steps > remaining) steps = remaining;
  _fadeStep = static_cast<uint16_t>(_fadeStep + steps);
  _lastStepMs += steps * static_cast<unsigned long>(_stepDelayMs);

  const float progress =
      static_cast<float>(_fadeStep) / static_cast<float>(cfg::kFadeSteps);
  applyPercent(_fadeStartPercent +
               (_fadeTargetPercent - _fadeStartPercent) * progress);

  if (_fadeStep < cfg::kFadeSteps) return;

  _fading = false;
  applyPercent(_fadeTargetPercent);

  if (!_breathing) return;

  if (_breathPhaseDown) {
    // Bottom of the breath reached: immediately climb back to this panel's
    // own brightness (not the global one).
    _breathPhaseDown = false;
    fadeTo(_panelBrightnessPercent, _breathDurationSec);
  } else {
    _breathing = false;
  }
}

void LedPanel::fadeTo(float targetPercent, float durationSec) {
  targetPercent = cie1931::clampPercent(targetPercent);

  if (durationSec < 0.0f) {
    durationSec = config.fadeTimeSec();
  }
  if (durationSec < kMinFadeSec) {
    durationSec = kMinFadeSec;
  }

  if (!_initialized) {
    _currentPercent = targetPercent;
    _fading = false;
    return;
  }

  if (!_breathing &&
      fabsf(targetPercent - _currentPercent) <= 0.005f) {
    applyPercent(targetPercent);
    _fading = false;
    return;
  }

  int delayMs =
      static_cast<int>((durationSec * 1000.0f) / static_cast<float>(cfg::kFadeSteps));
  if (delayMs < 1) delayMs = 1;

  _fadeStartPercent = _currentPercent;
  _fadeTargetPercent = targetPercent;
  _fadeStep = 0;
  _stepDelayMs = delayMs;
  _lastStepMs = millis();
  _fading = true;
}

void LedPanel::applyPercent(float percent) {
  _currentPercent = cie1931::clampPercent(percent);
  _currentDuty = cie1931::percentToDuty(_currentPercent);
  writeDuty(_currentDuty);
}

void LedPanel::writeDuty(uint16_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(_pin, duty);
#else
  ledcWrite(_channel, duty);
#endif
}

bool LedPanel::consumeStateChanged() {
  const bool changed = _stateChanged;
  _stateChanged = false;
  return changed;
}

bool LedPanel::consumeBrightnessChanged() {
  const bool changed = _brightnessChanged;
  _brightnessChanged = false;
  return changed;
}
