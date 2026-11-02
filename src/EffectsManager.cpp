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

  _sunsetActive = false;
  _sunsetStartMs = 0;
  _sunsetLastStepMs = 0;
  for (uint8_t i = 0; i < cfg::kPanelCount; ++i) {
    _sunsetStartPercent[i] = 0.0f;
    _sunsetPanelActive[i] = false;
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
  updateSunset();
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
  cancelSunset();
}

void EffectsManager::onManualOverrideForPanel(uint8_t panelIndex) {
  cancelTransientEffects();
  if (_sunriseActive) {
    stopSunrise();
  }
  _wave.active = false;
  cancelSunsetPanel(panelIndex);
}

bool EffectsManager::startSunrise() {
  if (_panels == nullptr || _sunriseActive || _sunsetActive) {
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

  if (_sunsetActive) {
    cancelSunset();
    if (!on) {
      // An explicit full OFF is immediate and wins over the sunset.
      turnAllPanelsOffImmediately();
      return;
    }
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

  bool topSunset = false;
  if (_sunsetActive) {
    for (uint8_t i = 2; i < _panelCount; ++i) {
      if (_sunsetPanelActive[i]) {
        topSunset = true;
        break;
      }
    }
  }

  if (topSunset) {
    // Remove the top panels from the sunset. The bottom pair keeps fading.
    for (uint8_t i = 2; i < _panelCount; ++i) {
      cancelSunsetPanel(i);
    }
    if (!on) {
      // Explicit top OFF is immediate, not a normal cascade.
      for (uint8_t i = 2; i < _panelCount; ++i) {
        _panels[i].setOn(false, cfg::kSunsetQuickOffSec);
        if (_network != nullptr) {
          _network->publishPanelState(_panels[i]);
        }
      }
      if (_network != nullptr) {
        _network->publishGroupStates();
        _network->publishStatus();
      }
      return;
    }
  }

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

  const uint8_t bottomCount = (_panelCount < 2) ? _panelCount : 2;
  bool bottomSunset = false;
  if (_sunsetActive) {
    for (uint8_t i = 0; i < bottomCount; ++i) {
      if (_sunsetPanelActive[i]) {
        bottomSunset = true;
        break;
      }
    }
  }

  if (bottomSunset) {
    // Remove the bottom pair from the sunset. The top panels keep fading.
    for (uint8_t i = 0; i < bottomCount; ++i) {
      cancelSunsetPanel(i);
    }
    if (!on) {
      for (uint8_t i = 0; i < bottomCount; ++i) {
        _panels[i].setOn(false, cfg::kSunsetQuickOffSec);
        if (_network != nullptr) {
          _network->publishPanelState(_panels[i]);
        }
      }
      if (_network != nullptr) {
        _network->publishGroupStates();
        _network->publishStatus();
      }
      return;
    }
  }

  uint8_t sequence[cfg::kPanelCount];
  uint8_t count = 0;
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

bool EffectsManager::startSunset() {
  if (_panels == nullptr || _panelCount == 0 || _sunsetActive ||
      _sunriseActive) {
    return false;
  }

  uint8_t activeCount = 0;
  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (_panels[i].isOn()) {
      ++activeCount;
    }
  }
  if (activeCount == 0) {
    Serial.println("EFFECTS: sunset rejected, no panels are on");
    return false;
  }

  _wave.active = false;
  cancelTransientEffects();

  _sunsetActive = true;
  _sunsetStartMs = millis();
  _sunsetLastStepMs = _sunsetStartMs;

  for (uint8_t i = 0; i < _panelCount; ++i) {
    const bool active = _panels[i].isOn();
    _sunsetPanelActive[i] = active;
    // The setpoint is the user-visible brightness shown in HA. Fading from it
    // is stable even if a short effect was running at 22:00.
    _sunsetStartPercent[i] =
        active ? _panels[i].brightnessPercent() : 0.0f;
    if (active) {
      _panels[i].setSunsetPercent(_sunsetStartPercent[i]);
    }
  }

  Serial.printf("EFFECTS: sunset started, %u panel(s), 2 hours\n",
                static_cast<unsigned>(activeCount));
  if (_network != nullptr) {
    _network->setSunsetActive(true);
    _network->publishSunsetState();
    _network->publishStatus();
  }
  return true;
}

void EffectsManager::stopSunset() {
  // Without an active sunset this is just a normal full-shelf OFF.
  if (!_sunsetActive) {
    setFullLight(false);
    return;
  }

  cancelSunset();
  turnAllPanelsOffImmediately();
}

void EffectsManager::updateSunset() {
  if (!_sunsetActive) {
    return;
  }

  if (!anySunsetPanelActive()) {
    cancelSunset();
    return;
  }

  const unsigned long now = millis();
  const unsigned long elapsed = now - _sunsetStartMs;

  if (elapsed >= cfg::kSunsetDurationMs) {
    finishSunset();
    return;
  }

  if (now - _sunsetLastStepMs < cfg::kSunsetStepMs) {
    return;
  }
  _sunsetLastStepMs = now;

  const float progress =
      static_cast<float>(elapsed) / static_cast<float>(cfg::kSunsetDurationMs);
  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (!_sunsetPanelActive[i]) {
      continue;
    }
    _panels[i].setSunsetPercent(_sunsetStartPercent[i] * (1.0f - progress));
  }
}

void EffectsManager::finishSunset() {
  if (!_sunsetActive) {
    return;
  }

  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (!_sunsetPanelActive[i]) {
      continue;
    }
    _panels[i].finishSunset();
    _sunsetPanelActive[i] = false;
    if (_network != nullptr) {
      _network->publishPanelState(_panels[i]);
    }
  }

  _sunsetActive = false;
  Serial.println("EFFECTS: sunset complete, panels off");
  if (_network != nullptr) {
    _network->setSunsetActive(false);
    _network->publishSunsetState();
    _network->publishGroupStates();
    _network->publishStatus();
  }
}

void EffectsManager::cancelSunset() {
  if (!_sunsetActive) {
    return;
  }

  _sunsetActive = false;
  for (bool& active : _sunsetPanelActive) {
    active = false;
  }
  Serial.println("EFFECTS: sunset cancelled");
  if (_network != nullptr) {
    _network->setSunsetActive(false);
    _network->publishSunsetState();
    _network->publishStatus();
  }
}

void EffectsManager::cancelSunsetPanel(uint8_t panelIndex) {
  if (!_sunsetActive || panelIndex >= _panelCount ||
      !_sunsetPanelActive[panelIndex]) {
    return;
  }

  _sunsetPanelActive[panelIndex] = false;
  if (!anySunsetPanelActive()) {
    _sunsetActive = false;
    Serial.println("EFFECTS: sunset stopped, no active panels left");
    if (_network != nullptr) {
      _network->setSunsetActive(false);
      _network->publishSunsetState();
      _network->publishStatus();
    }
  }
}

bool EffectsManager::anySunsetPanelActive() const {
  for (bool active : _sunsetPanelActive) {
    if (active) return true;
  }
  return false;
}

void EffectsManager::turnAllPanelsOffImmediately() {
  _wave.active = false;
  cancelTransientEffects();
  for (uint8_t i = 0; i < _panelCount; ++i) {
    _panels[i].setOn(false, cfg::kSunsetQuickOffSec);
    if (_network != nullptr) {
      _network->publishPanelState(_panels[i]);
    }
  }
  if (_network != nullptr) {
    _network->publishGroupStates();
    _network->publishStatus();
  }
}

bool EffectsManager::startBreathingWave() {
  if (_panels == nullptr || _panelCount == 0 || _breathing.active ||
      _sunriseActive || _sunsetActive || _wave.active || _glitch.active) {
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

  if (_sunriseActive || _sunsetActive || _wave.active) {
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

    if (_sunriseActive || _sunsetActive || _wave.active ||
        _breathing.active) {
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
