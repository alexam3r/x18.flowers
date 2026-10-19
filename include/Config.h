#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include <cmath>

// ---------------------------------------------------------------------------
// Compile-time configuration.
//
// Everything in this header is a constant of the physical installation:
// pins, PWM parameters, MQTT topic contract and timing defaults. Mutable,
// user-tunable settings (fade / interval) live in NVS and are handled by
// the Config class below.
// ---------------------------------------------------------------------------

namespace cfg {

// --- Hardware --------------------------------------------------------------
// Shelf 1 is the bottom shelf, shelf 5 is the top one.
// ESP32-C3 SuperMini: GPIO 2/8/9 are strapping pins and must stay unused.
constexpr uint8_t kPanelCount = 5;
constexpr uint8_t kPanelPins[kPanelCount] = {0, 1, 3, 4, 10};

// --- PWM -------------------------------------------------------------------
// Hardware LEDC limit at 4 kHz on ESP32-C3 is 14 bit; 12 bit is the contract.
constexpr uint32_t kPwmFreqHz = 4000;
constexpr uint8_t kPwmResolutionBits = 12;
constexpr uint32_t kPwmMaxDuty = (1u << kPwmResolutionBits) - 1u;  // 4095
constexpr uint16_t kFadeSteps = 200;

// --- Network ---------------------------------------------------------------
constexpr char kClientId[] = "flowers-esp32";
constexpr uint16_t kMqttPort = 1883;
constexpr char kTopicBase[] = "home/flat7/flowers/";

// Command topics.
constexpr char kTopicBrightness[] = "home/flat7/flowers/brightness";
constexpr char kTopicFullLight[] = "home/flat7/flowers/fullLight";
constexpr char kTopicTopLight[] = "home/flat7/flowers/topLight";
constexpr char kTopicBottomLight[] = "home/flat7/flowers/bottomLight";
constexpr char kTopicSunrise[] = "home/flat7/flowers/sunrise";
constexpr char kTopicStartWave[] = "home/flat7/flowers/startWave";
constexpr char kTopicConfigFadeTime[] = "home/flat7/flowers/config/fadeTime";
constexpr char kTopicConfigInterval[] = "home/flat7/flowers/config/interval";
constexpr char kTopicConfigReboot[] = "home/flat7/flowers/config/reboot";

// State / telemetry topics.
constexpr char kTopicStatus[] = "home/flat7/flowers/status";
constexpr char kTopicFullLightState[] = "home/flat7/flowers/fullLight_state";
constexpr char kTopicTopLightState[] = "home/flat7/flowers/topLight_state";
constexpr char kTopicBottomLightState[] = "home/flat7/flowers/bottomLight_state";
constexpr char kTopicSunriseState[] = "home/flat7/flowers/sunrise_state";
constexpr char kTopicStartWaveState[] = "home/flat7/flowers/startWave_state";
constexpr char kTopicBrightnessState[] = "home/flat7/flowers/brightness_state";

// Per-panel topic pieces:
//   <kTopicPanelPrefix><id>[kTopicPanelStateSuffix]
//   <kTopicPanelPrefix><id>[kTopicPanelBrightnessSuffix]
constexpr char kTopicPanelPrefix[] = "home/flat7/flowers/panel_";
constexpr char kTopicPanelStateSuffix[] = "_state";
constexpr char kTopicPanelBrightnessSuffix[] = "/brightness";
constexpr char kTopicPanelBrightnessStateSuffix[] = "/brightness_state";

// --- Timings ---------------------------------------------------------------
constexpr unsigned long kBootUsbDelayMs = 5000;
constexpr unsigned long kTelemetryIntervalMs = 10000;
constexpr unsigned long kConnectionWatchdogMs = 180000;
constexpr unsigned long kWifiConnectRetryMs = 500;
constexpr uint8_t kWifiConnectRetries = 20;

// Sunrise: 5 panels, one every 2 minutes, 6 minutes per fade => 14 minutes.
constexpr unsigned long kSunrisePanelIntervalMs = 120000;
constexpr float kSunriseFadeSec = 360.0f;
constexpr unsigned long kSunriseDurationMs = 14UL * 60UL * 1000UL;

// Breathing wave scheduling window (8..10 minutes).
constexpr int kBreathingMinSec = 480;
constexpr int kBreathingMaxSec = 600;

// Glitch scheduling window (3..4 minutes) and duration.
constexpr unsigned long kGlitchMinIntervalMs = 180000;
constexpr unsigned long kGlitchMaxIntervalMs = 240000;
constexpr unsigned long kGlitchMinDurationMs = 200;
constexpr unsigned long kGlitchMaxDurationMs = 500;

// --- Mutable settings (NVS) ------------------------------------------------
constexpr char kNvsNamespace[] = "flowers";
constexpr char kNvsKeyFadeTime[] = "fadeTime";
constexpr char kNvsKeyWaveInterval[] = "interval";

constexpr float kDefaultFadeTimeSec = 2.0f;
constexpr float kMinFadeTimeSec = 0.1f;
constexpr float kMaxFadeTimeSec = 10.0f;

constexpr float kDefaultWaveIntervalSec = 0.5f;
constexpr float kMinWaveIntervalSec = 0.0f;
constexpr float kMaxWaveIntervalSec = 5.0f;

constexpr int kDefaultBrightnessPercent = 100;
constexpr int kMinBrightnessPercent = 1;
constexpr int kMaxBrightnessPercent = 100;

}  // namespace cfg

// Settings persisted in the "flowers" NVS namespace.
struct DeviceSettings {
  float fadeTimeSec = cfg::kDefaultFadeTimeSec;
  float waveIntervalSec = cfg::kDefaultWaveIntervalSec;
};

// Reads and writes the user-tunable settings from/to NVS (Preferences).
// Values are validated against the ranges above before they are persisted.
class Config {
 public:
  // Opens the NVS namespace and loads the stored settings. Missing or
  // out-of-range values fall back to the defaults. Returns true if NVS is
  // available.
  bool begin();

  const DeviceSettings& settings() const { return _settings; }
  float fadeTimeSec() const { return _settings.fadeTimeSec; }
  float waveIntervalSec() const { return _settings.waveIntervalSec; }
  bool ready() const { return _ready; }

  // Validates, applies and persists the value. Returns false when the
  // value is out of range or NVS is not available.
  bool setFadeTimeSec(float seconds);
  bool setWaveIntervalSec(float seconds);

  // Restores and persists the compile-time defaults.
  bool resetToDefaults();

  static bool isValidFadeTime(float seconds) {
    return std::isfinite(seconds) && seconds >= cfg::kMinFadeTimeSec &&
           seconds <= cfg::kMaxFadeTimeSec;
  }
  static bool isValidWaveInterval(float seconds) {
    return std::isfinite(seconds) && seconds >= cfg::kMinWaveIntervalSec &&
           seconds <= cfg::kMaxWaveIntervalSec;
  }

 private:
  bool persistFloat(const char* key, float value);

  Preferences _prefs;
  DeviceSettings _settings;
  bool _ready = false;
};

// Single process-wide configuration instance.
extern Config config;
