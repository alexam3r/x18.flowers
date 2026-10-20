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
}

void EffectsManager::update() {
  if (_panels == nullptr) {
    return;
  }
  updateSunrise();
  updateWave();
}

void EffectsManager::onManualOverride() {
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
