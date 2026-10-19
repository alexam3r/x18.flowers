#include "Config.h"

#include <Preferences.h>

Config config;

bool Config::begin() {
  _settings = DeviceSettings{};
  _ready = _prefs.begin(cfg::kNvsNamespace, false);
  if (!_ready) {
    Serial.println("CONFIG: NVS unavailable, using defaults");
    return false;
  }

  const float storedFade =
      _prefs.getFloat(cfg::kNvsKeyFadeTime, cfg::kDefaultFadeTimeSec);
  const float storedInterval =
      _prefs.getFloat(cfg::kNvsKeyWaveInterval, cfg::kDefaultWaveIntervalSec);

  _settings.fadeTimeSec =
      isValidFadeTime(storedFade) ? storedFade : cfg::kDefaultFadeTimeSec;
  _settings.waveIntervalSec = isValidWaveInterval(storedInterval)
                                  ? storedInterval
                                  : cfg::kDefaultWaveIntervalSec;

  Serial.printf("CONFIG: loaded fade=%.2f s, interval=%.2f s\n",
                _settings.fadeTimeSec, _settings.waveIntervalSec);
  return true;
}

bool Config::persistFloat(const char* key, float value) {
  if (!_ready) return false;
  return _prefs.putFloat(key, value) == sizeof(float);
}

bool Config::setFadeTimeSec(float seconds) {
  if (!isValidFadeTime(seconds)) return false;
  if (!persistFloat(cfg::kNvsKeyFadeTime, seconds)) return false;
  _settings.fadeTimeSec = seconds;
  return true;
}

bool Config::setWaveIntervalSec(float seconds) {
  if (!isValidWaveInterval(seconds)) return false;
  if (!persistFloat(cfg::kNvsKeyWaveInterval, seconds)) return false;
  _settings.waveIntervalSec = seconds;
  return true;
}

bool Config::resetToDefaults() {
  if (!_ready) return false;
  const bool fadeSaved =
      persistFloat(cfg::kNvsKeyFadeTime, cfg::kDefaultFadeTimeSec);
  const bool intervalSaved = persistFloat(cfg::kNvsKeyWaveInterval,
                                          cfg::kDefaultWaveIntervalSec);
  if (!fadeSaved || !intervalSaved) return false;
  _settings = DeviceSettings{};
  return true;
}
