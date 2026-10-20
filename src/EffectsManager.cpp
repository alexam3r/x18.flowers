#include "EffectsManager.h"

#include "LedPanel.h"
#include "NetworkManager.h"

// 4 x 120 s between starts + 360 s fade = 840 s = 14 minutes.
static_assert(cfg::kSunriseDurationMs ==
                  static_cast<unsigned long>(cfg::kPanelCount - 1) *
                          cfg::kSunrisePanelIntervalMs +
                      static_cast<unsigned long>(cfg::kSunriseFadeSec * 1000.0f),
              "Sunrise schedule must remain 14 minutes");

void EffectsManager::begin(LedPanel* panels, uint8_t panelCount,
                           FlowersNetworkManager* network) {
  _panels = panels;
  _panelCount =
      (panelCount > cfg::kPanelCount) ? cfg::kPanelCount : panelCount;
  _network = network;

  _sunriseActive = false;
  _sunriseStartMs = 0;
  for (bool& started : _sunriseStarted) {
    started = false;
  }
  _wave.active = false;

  _breathing.active = false;
  _breathing.count = 0;
  _breathing.index = 0;
  _breathing.lastStepMs = 0;
  resetBreathingSchedule();

  _glitch.active = false;
  scheduleNextGlitch();
}

void EffectsManager::update() {
  if (_panels == nullptr) {
    return;
  }

  updateSunrise();
  updateWave();

  if (!_breathing.active &&
      millis() - _breathingLastMs >= _breathingIntervalMs) {
    if (!startBreathingWave()) {
      resetBreathingSchedule();
    }
  }
  updateBreathingWave();
  updateGlitch();
}

void EffectsManager::onManualOverride() {
  cancelTransientEffects();
  if (_sunriseActive) {
    stopSunrise();
  }
  _wave.active = false;
}

bool EffectsManager::startSunrise() {
  if (_panels == nullptr || _sunriseActive) {
    return false;
  }

  // The sunrise is allowed only from a completely dark shelf. A panel that is
  // still fading down counts as not ready.
  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (_panels[i].isOn() || _panels[i].isFading()) {
      Serial.println("EFFECTS: sunrise rejected, panels are not off");
      return false;
    }
  }

  _wave.active = false;
  cancelTransientEffects();
  _sunriseActive = true;
  _sunriseStartMs = millis();
  for (bool& started : _sunriseStarted) {
    started = false;
  }

  Serial.println("EFFECTS: sunrise started");

  if (_network != nullptr) {
    _network->setSunriseActive(true);
    _network->setGlobalBrightnessPercent(100);

    // The sunrise always runs at 100 % perceived brightness.
    for (uint8_t i = 0; i < _panelCount; ++i) {
      _panels[i].setBrightnessPercent(100.0f);
      _network->publishPanelBrightness(_panels[i]);
    }
    _network->publishGlobalBrightness();
    _network->publishSunriseState();
    _network->publishStatus();
  }

  // Shelf 1 starts immediately; the remaining shelves are scheduled by
  // updateSunrise() every kSunrisePanelIntervalMs.
  updateSunrise();
  return true;
}

void EffectsManager::stopSunrise() {
  if (!_sunriseActive) {
    return;
  }

  _sunriseActive = false;
  Serial.println("EFFECTS: sunrise stopped");

  if (_network != nullptr) {
    _network->setSunriseActive(false);
  }

  for (uint8_t i = 0; i < _panelCount; ++i) {
    _panels[i].setOn(false, config.fadeTimeSec());
    if (_network != nullptr) {
      _network->publishPanelState(_panels[i]);
    }
  }

  if (_network != nullptr) {
    _network->publishGroupStates();
    _network->publishSunriseState();
    _network->publishStatus();
  }
}

void EffectsManager::setFullLight(bool on) {
  if (_panels == nullptr) {
    return;
  }

  if (on && _sunriseActive) {
    // The sunrise owns the shelf; do not interrupt it with a full ON cascade.
    return;
  }

  cancelTransientEffects();
  if (!on) {
    stopSunrise();
  }

  uint8_t sequence[cfg::kPanelCount];
  for (uint8_t i = 0; i < _panelCount; ++i) {
    sequence[i] = on ? i : static_cast<uint8_t>(_panelCount - 1 - i);
  }
  startWave(sequence, _panelCount, on);
}

void EffectsManager::setTopLight(bool on) {
  if (_panels == nullptr) {
    return;
  }

  // A top command always interrupts the sunrise (old firmware behaviour).
  cancelTransientEffects();
  stopSunrise();

  uint8_t sequence[cfg::kPanelCount];
  uint8_t count = 0;
  if (on) {
    for (uint8_t i = 2; i < _panelCount; ++i) {
      sequence[count++] = i;
    }
  } else {
    for (uint8_t i = _panelCount; i > 2; --i) {
      sequence[count++] = static_cast<uint8_t>(i - 1);
    }
  }
  startWave(sequence, count, on);
}

void EffectsManager::setBottomLight(bool on) {
  if (_panels == nullptr) {
    return;
  }

  cancelTransientEffects();
  stopSunrise();

  uint8_t sequence[cfg::kPanelCount];
  uint8_t count = 0;
  const uint8_t bottomCount = (_panelCount < 2) ? _panelCount : 2;
  if (on) {
    for (uint8_t i = 0; i < bottomCount; ++i) {
      sequence[count++] = i;
    }
  } else {
    for (uint8_t i = bottomCount; i > 0; --i) {
      sequence[count++] = static_cast<uint8_t>(i - 1);
    }
  }
  startWave(sequence, count, on);
}

void EffectsManager::startWave(const uint8_t* sequence, uint8_t count,
                               bool turnOn) {
  if (_panels == nullptr || sequence == nullptr || count == 0) {
    return;
  }
  if (count > _panelCount) {
    count = _panelCount;
  }

  _wave.active = true;
  _wave.count = count;
  _wave.index = 0;
  _wave.turnOn = turnOn;
  _wave.lastStepMs = millis();
  for (uint8_t i = 0; i < count; ++i) {
    _wave.sequence[i] = sequence[i];
  }

  // First shelf reacts immediately, the rest follow at waveIntervalSec.
  stepWave();
}

void EffectsManager::stepWave() {
  if (!_wave.active) {
    return;
  }
  if (_wave.index >= _wave.count) {
    _wave.active = false;
    return;
  }

  const uint8_t panelIndex = _wave.sequence[_wave.index++];
  if (panelIndex >= _panelCount) {
    _wave.active = false;
    return;
  }

  LedPanel& panel = _panels[panelIndex];
  panel.setOn(_wave.turnOn, config.fadeTimeSec());

  if (_network != nullptr) {
    _network->publishPanelState(panel);
    _network->publishGroupStates();
  }

  if (_wave.index >= _wave.count) {
    _wave.active = false;
  }
}

void EffectsManager::updateWave() {
  if (!_wave.active) {
    return;
  }

  const unsigned long intervalMs =
      static_cast<unsigned long>(config.waveIntervalSec() * 1000.0f);
  if (millis() - _wave.lastStepMs < intervalMs) {
    return;
  }

  _wave.lastStepMs = millis();
  stepWave();
}

void EffectsManager::updateSunrise() {
  if (!_sunriseActive) {
    return;
  }

  const unsigned long elapsed = millis() - _sunriseStartMs;
  bool startedAny = false;

  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (_sunriseStarted[i]) {
      continue;
    }
    const unsigned long threshold =
        static_cast<unsigned long>(i) * cfg::kSunrisePanelIntervalMs;
    if (elapsed < threshold) {
      continue;
    }

    _sunriseStarted[i] = true;
    _panels[i].setOn(true, cfg::kSunriseFadeSec);
    startedAny = true;

    if (_network != nullptr) {
      _network->publishPanelState(_panels[i]);
    }
  }

  if (startedAny && _network != nullptr) {
    _network->publishGroupStates();
  }

  // 4 x 120 s between starts + 360 s fade = 840 s (14 minutes).
  if (elapsed >= cfg::kSunriseDurationMs) {
    _sunriseActive = false;
    Serial.println("EFFECTS: sunrise complete");

    if (_network != nullptr) {
      _network->setSunriseActive(false);
      _network->publishSunriseState();
      _network->publishStatus();
    }
  }
}

bool EffectsManager::startBreathingWave() {
  if (_panels == nullptr || _panelCount == 0 || _breathing.active ||
      _sunriseActive || _wave.active || _glitch.active) {
    return false;
  }

  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (!_panels[i].isOn()) {
      return false;
    }
  }

  const bool up = random(0, 2) != 0;
  _breathing.count = _panelCount;
  _breathing.index = 0;
  for (uint8_t i = 0; i < _panelCount; ++i) {
    _breathing.sequence[i] =
        up ? i : static_cast<uint8_t>(_panelCount - 1 - i);
  }
  _breathing.active = true;
  _breathing.lastStepMs = millis();
  resetBreathingSchedule();

  if (_network != nullptr) {
    _network->setStartWaveActive(true);
    _network->publishStartWaveState();
  }
  Serial.println("EFFECTS: breathing wave started");

  // First shelf reacts immediately, the rest follow at waveIntervalSec.
  stepBreathingWave();
  return _breathing.active;
}

void EffectsManager::stepBreathingWave() {
  if (!_breathing.active) {
    return;
  }
  if (_breathing.index >= _breathing.count) {
    return;
  }

  const uint8_t panelIndex = _breathing.sequence[_breathing.index++];
  if (panelIndex >= _panelCount || !_panels[panelIndex].isOn() ||
      !_panels[panelIndex].startBreathing(2.0f)) {
    cancelBreathingWave();
    return;
  }

  _breathing.lastStepMs = millis();
}

void EffectsManager::updateBreathingWave() {
  if (!_breathing.active) {
    return;
  }

  if (_sunriseActive || _wave.active) {
    cancelBreathingWave();
    return;
  }

  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (!_panels[i].isOn()) {
      cancelBreathingWave();
      return;
    }
  }

  if (_breathing.index < _breathing.count) {
    const unsigned long intervalMs =
        static_cast<unsigned long>(config.waveIntervalSec() * 1000.0f);
    if (millis() - _breathing.lastStepMs >= intervalMs) {
      stepBreathingWave();
    }
    return;
  }

  bool anyBreathing = false;
  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (_panels[i].isBreathing()) {
      anyBreathing = true;
      break;
    }
  }

  if (!anyBreathing) {
    finishBreathingWave();
  }
}

void EffectsManager::finishBreathingWave() {
  if (!_breathing.active) {
    return;
  }

  _breathing.active = false;
  _breathing.index = 0;
  _breathing.count = 0;
  resetBreathingSchedule();

  if (_network != nullptr) {
    _network->setStartWaveActive(false);
    _network->publishStartWaveState();
  }
  Serial.println("EFFECTS: breathing wave finished");
}

void EffectsManager::cancelBreathingWave() {
  if (!_breathing.active) {
    return;
  }

  _breathing.active = false;
  _breathing.index = 0;
  _breathing.count = 0;
  for (uint8_t i = 0; i < _panelCount; ++i) {
    _panels[i].cancelBreathing();
  }
  resetBreathingSchedule();

  if (_network != nullptr) {
    _network->setStartWaveActive(false);
    _network->publishStartWaveState();
  }
  Serial.println("EFFECTS: breathing wave cancelled");
}

void EffectsManager::updateGlitch() {
  const unsigned long now = millis();

  if (!_glitch.active) {
    if (now - _glitchLastMs < _glitchIntervalMs) {
      return;
    }
    scheduleNextGlitch();

    if (_sunriseActive || _wave.active || _breathing.active) {
      return;
    }

    uint8_t candidates[cfg::kPanelCount];
    uint8_t count = 0;
    for (uint8_t i = 0; i < _panelCount; ++i) {
      if (_panels[i].isOn() && !_panels[i].isFading()) {
        candidates[count++] = i;
      }
    }
    if (count == 0) {
      return;
    }

    const uint8_t panelIndex = candidates[random(0, count)];
    _glitch.active = true;
    _glitch.panelIndex = panelIndex;
    _glitch.originalDuty = _panels[panelIndex].currentDuty();
    _glitch.endMs =
        now + static_cast<unsigned long>(
                  random(static_cast<long>(cfg::kGlitchMinDurationMs),
                         static_cast<long>(cfg::kGlitchMaxDurationMs)));
    _glitch.nextFlickerMs = now;
    return;
  }

  if (!_panels[_glitch.panelIndex].isOn() ||
      _panels[_glitch.panelIndex].isFading()) {
    cancelGlitch();
    return;
  }

  if (static_cast<long>(now - _glitch.endMs) >= 0) {
    _panels[_glitch.panelIndex].setRawPwm(_glitch.originalDuty);
    _glitch.active = false;
    scheduleNextGlitch();
    return;
  }

  if (static_cast<long>(now - _glitch.nextFlickerMs) >= 0) {
    uint16_t duty = 0;
    if (random(0, 10) > 5) {
      duty = 0;
    } else {
      duty = static_cast<uint16_t>(_glitch.originalDuty / random(2, 5));
    }
    _panels[_glitch.panelIndex].setRawPwm(duty);
    _glitch.nextFlickerMs = now + static_cast<unsigned long>(random(30, 100));
  }
}

void EffectsManager::cancelGlitch() {
  if (!_glitch.active) {
    return;
  }

  _panels[_glitch.panelIndex].setRawPwm(_glitch.originalDuty);
  _glitch.active = false;
  scheduleNextGlitch();
  Serial.println("EFFECTS: glitch cancelled, PWM restored");
}

void EffectsManager::cancelTransientEffects() {
  cancelGlitch();
  cancelBreathingWave();
}

void EffectsManager::resetBreathingSchedule() {
  _breathingLastMs = millis();
  _breathingIntervalMs =
      static_cast<unsigned long>(
          random(cfg::kBreathingMinSec, cfg::kBreathingMaxSec)) *
      1000UL;
}

void EffectsManager::scheduleNextGlitch() {
  _glitchLastMs = millis();
  _glitchIntervalMs = static_cast<unsigned long>(
      random(static_cast<long>(cfg::kGlitchMinIntervalMs),
             static_cast<long>(cfg::kGlitchMaxIntervalMs)));
}
