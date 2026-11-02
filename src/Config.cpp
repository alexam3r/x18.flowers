#include "Config.h"

#include <Preferences.h>

#include <cstring>

namespace {

bool validBrightness(uint8_t value) {
  return value >= cfg::kMinBrightnessPercent &&
         value <= cfg::kMaxBrightnessPercent;
}

bool validPanelBrightness(const PanelBrightnessSettings& settings) {
  if (!validBrightness(settings.globalPercent)) return false;
  for (uint8_t value : settings.panelPercent) {
    if (!validBrightness(value)) return false;
  }
  return true;
}

}  // namespace

Config config;

bool Config::begin() {
  _settings = DeviceSettings{};
  _panelBrightness = PanelBrightnessSettings{};
  _ready = _prefs.begin(cfg::kNvsNamespace, false);
  if (!_ready) {
    Serial.println("CONFIG: NVS unavailable, using defaults");
    return false;
  }

  loadPanelBrightness();

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

bool Config::loadPanelBrightness() {
  PanelBrightnessSettings stored;
  const size_t read =
      _prefs.getBytes(cfg::kNvsKeyPanelBrightness, &stored, sizeof(stored));
  if (read != sizeof(stored)) {
    Serial.println("CONFIG: no stored panel brightness, using 100%");
    return false;
  }

  if (stored.version != 1 || !validPanelBrightness(stored)) {
    Serial.println("CONFIG: invalid panel brightness in NVS, using 100%");
    return false;
  }

  _panelBrightness = stored;
  Serial.printf(
      "CONFIG: loaded panel brightness p1=%u p2=%u p3=%u p4=%u p5=%u, "
      "global=%u%%\n",
      static_cast<unsigned>(_panelBrightness.panelPercent[0]),
      static_cast<unsigned>(_panelBrightness.panelPercent[1]),
      static_cast<unsigned>(_panelBrightness.panelPercent[2]),
      static_cast<unsigned>(_panelBrightness.panelPercent[3]),
      static_cast<unsigned>(_panelBrightness.panelPercent[4]),
      static_cast<unsigned>(_panelBrightness.globalPercent));
  return true;
}

bool Config::savePanelBrightness(const PanelBrightnessSettings& settings) {
  if (!_ready) return false;

  PanelBrightnessSettings next = settings;
  next.version = 1;
  if (!validPanelBrightness(next)) return false;

  if (next.globalPercent == _panelBrightness.globalPercent &&
      std::memcmp(next.panelPercent, _panelBrightness.panelPercent,
                  sizeof(next.panelPercent)) == 0) {
    return true;  // nothing changed
  }

  if (_prefs.putBytes(cfg::kNvsKeyPanelBrightness, &next, sizeof(next)) !=
      sizeof(next)) {
    return false;
  }

  _panelBrightness = next;
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
  const PanelBrightnessSettings defaults;
  const bool brightnessSaved =
      _prefs.putBytes(cfg::kNvsKeyPanelBrightness, &defaults,
                      sizeof(defaults)) == sizeof(defaults);
  if (!fadeSaved || !intervalSaved || !brightnessSaved) return false;
  _settings = DeviceSettings{};
  _panelBrightness = defaults;
  return true;
}
