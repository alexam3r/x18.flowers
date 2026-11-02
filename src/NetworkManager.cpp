#include "NetworkManager.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "EffectsManager.h"
#include "LedPanel.h"
#include "ResetReason.h"
#include "Secrets.h"

namespace {
constexpr unsigned long kMqttRetryIntervalMs = 5000;
constexpr unsigned long kWifiReconnectIntervalMs = 10000;
// Brightness is a rare user action; a short debounce coalesces slider updates
// without leaving a long window where a power cut loses the last change.
constexpr unsigned long kBrightnessSaveDelayMs = 2000;
constexpr uint16_t kMqttBufferSize = 512;
constexpr uint16_t kMqttSocketTimeoutSec = 2;

bool parseLong(const String& text, long& value) {
  if (text.length() == 0) return false;
  char* end = nullptr;
  value = std::strtol(text.c_str(), &end, 10);
  return end != text.c_str() && end != nullptr && *end == '\0';
}

bool parseFloat(const String& text, float& value) {
  if (text.length() == 0) return false;
  char* end = nullptr;
  value = std::strtof(text.c_str(), &end);
  return end != text.c_str() && end != nullptr && *end == '\0';
}
}  // namespace

FlowersNetworkManager::FlowersNetworkManager() : _mqtt(_wifiClient) {}

bool FlowersNetworkManager::begin(LedPanel* panels, uint8_t panelCount) {
  _panels = panels;
  _panelCount = (panelCount > cfg::kPanelCount) ? cfg::kPanelCount : panelCount;

  _mqtt.setBufferSize(kMqttBufferSize);
  _mqtt.setSocketTimeout(kMqttSocketTimeoutSec);
  _mqtt.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
    handleMessage(topic, payload, length);
  });

  _lastConnectionMs = millis();
  _lastTelemetryMs = millis();
  _lastWifiReconnectMs = millis();
  _pendingBrightness = config.panelBrightness();

  const bool wifi = connectWifi();
  _lastConnectionMs = millis();
  _wifiConnected = wifi;
  if (wifi) {
    resolveBroker();
  } else {
    Serial.println("NET: Wi-Fi is not connected at boot, will retry");
  }
  return wifi;
}

void FlowersNetworkManager::update() {
  // Also runs while Wi-Fi/MQTT is down: a user command must reach NVS after
  // the debounce window even if the broker disappears immediately afterwards.
  flushBrightnessSave();

  if (WiFi.status() != WL_CONNECTED) {
    if (_wifiConnected) {
      _wifiConnected = false;
      if (_mqtt.connected()) {
        _mqtt.disconnect();
      }
      Serial.println("NET: Wi-Fi lost");
    }
    maybeReconnectWifi();
    checkWatchdog();
    return;
  }

  if (!_wifiConnected) {
    _wifiConnected = true;
    Serial.println("NET: Wi-Fi connected");
    resolveBroker();
    _firstRunStatus = true;
  }

  if (!_mqtt.connected()) {
    tryConnectMqtt();
  }

  const bool mqttNowConnected = _mqtt.connected();
  if (_mqttWasConnected && !mqttNowConnected) {
    Serial.printf("NET: MQTT disconnected (state=%d)\n", _mqtt.state());
  }
  _mqttWasConnected = mqttNowConnected;

  if (mqttNowConnected) {
    _mqtt.loop();
    publishGroupStates();

    if (millis() - _lastTelemetryMs >= cfg::kTelemetryIntervalMs) {
      _lastTelemetryMs = millis();
      publishStatus();
    }
  }

  checkWatchdog();
}

bool FlowersNetworkManager::wifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool FlowersNetworkManager::mqttConnected() {
  return _mqtt.connected();
}

void FlowersNetworkManager::setGlobalBrightnessPercent(int percent) {
  if (percent < cfg::kMinBrightnessPercent) {
    percent = cfg::kMinBrightnessPercent;
  }
  if (percent > cfg::kMaxBrightnessPercent) {
    percent = cfg::kMaxBrightnessPercent;
  }
  _globalBrightnessPercent = percent;
}

bool FlowersNetworkManager::connectWifi() {
  WiFi.mode(WIFI_STA);
  // The C3 SuperMini has a weak ceramic antenna: modem sleep loses DHCP/MQTT
  // packets, so both sleep and TX power are fixed explicitly.
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("NET: connecting Wi-Fi");

  for (uint8_t attempt = 0; attempt < cfg::kWifiConnectRetries; ++attempt) {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(cfg::kWifiConnectRetryMs);
    Serial.print('.');
  }
  Serial.println();

  return WiFi.status() == WL_CONNECTED;
}

void FlowersNetworkManager::resolveBroker() {
  _brokerIp = WiFi.gatewayIP();
  _mqtt.setServer(_brokerIp, cfg::kMqttPort);
  Serial.printf("NET: MQTT broker (gateway) %s:%u, client id=%s\n",
                _brokerIp.toString().c_str(),
                static_cast<unsigned>(cfg::kMqttPort), cfg::kClientId);
}

void FlowersNetworkManager::maybeReconnectWifi() {
  const unsigned long now = millis();
  if (now - _lastWifiReconnectMs < kWifiReconnectIntervalMs) {
    return;
  }
  _lastWifiReconnectMs = now;
  Serial.println("NET: Wi-Fi reconnect attempt");
  WiFi.reconnect();
}

void FlowersNetworkManager::tryConnectMqtt() {
  const unsigned long now = millis();
  if (now - _lastMqttAttemptMs < kMqttRetryIntervalMs) {
    return;
  }
  _lastMqttAttemptMs = now;

  Serial.printf("NET: MQTT connecting as %s...\n", cfg::kClientId);
  if (!_mqtt.connect(cfg::kClientId, MQTT_USER, MQTT_PASS)) {
    Serial.printf("NET: MQTT connect failed, rc=%d\n", _mqtt.state());
    return;
  }

  Serial.println("NET: MQTT connected");

  // Must happen before subscribing: the old firmware retained panel states on
  // panel_N, and command topics such as fullLight/sunrise may also have stale
  // retained values. PubSubClient does not expose the retain flag, so such a
  // message is indistinguishable from a fresh command. Commands are never
  // supposed to be retained; clear them first so the broker cannot replay
  // them on this or any later reconnect.
  clearRetainedCommands();

  const String subscription = String(cfg::kTopicBase) + "#";
  if (!_mqtt.subscribe(subscription.c_str())) {
    Serial.println("NET: MQTT subscription failed");
  }

  _firstRunStatus = true;
  publishAllStates();
  _lastTelemetryMs = millis();
}

void FlowersNetworkManager::clearRetainedCommands() {
  static const char* const kSimpleCommands[] = {
      cfg::kTopicBrightness,     cfg::kTopicFullLight,
      cfg::kTopicTopLight,       cfg::kTopicBottomLight,
      cfg::kTopicSunrise,        cfg::kTopicSunset,
      cfg::kTopicStartWave,      cfg::kTopicConfigFadeTime,
      cfg::kTopicConfigInterval, cfg::kTopicConfigReboot,
  };

  uint8_t failed = 0;
  for (const char* topic : kSimpleCommands) {
    if (!_mqtt.publish(topic, "", true)) {
      ++failed;
    }
  }

  for (uint8_t i = 0; i < _panelCount; ++i) {
    const String base =
        String(cfg::kTopicPanelPrefix) + String(static_cast<unsigned>(i + 1));
    if (!_mqtt.publish(base.c_str(), "", true)) {
      ++failed;
    }

    const String brightness = base + cfg::kTopicPanelBrightnessSuffix;
    if (!_mqtt.publish(brightness.c_str(), "", true)) {
      ++failed;
    }
  }

  if (failed != 0) {
    Serial.printf("NET: retained command cleanup: %u publish(es) failed\n",
                  static_cast<unsigned>(failed));
  } else {
    Serial.println("NET: retained command topics cleared");
  }
}

void FlowersNetworkManager::rememberPanelBrightness(uint8_t index,
                                                     uint8_t percent) {
  if (index >= cfg::kPanelCount) return;
  _pendingBrightness.panelPercent[index] = percent;
  _brightnessDirty = true;
  _brightnessDirtyMs = millis();
}

void FlowersNetworkManager::rememberGlobalBrightness(uint8_t percent) {
  _pendingBrightness.globalPercent = percent;
  for (uint8_t& panelPercent : _pendingBrightness.panelPercent) {
    panelPercent = percent;
  }
  _brightnessDirty = true;
  _brightnessDirtyMs = millis();
}

void FlowersNetworkManager::flushBrightnessSave(bool force) {
  if (!_brightnessDirty || _panels == nullptr) return;
  if (!force && millis() - _brightnessDirtyMs < kBrightnessSaveDelayMs) {
    return;
  }

  if (!config.savePanelBrightness(_pendingBrightness)) {
    Serial.println("CONFIG: failed to save panel brightness, will retry");
    _brightnessDirtyMs = millis();
    return;
  }

  _brightnessDirty = false;
  Serial.printf(
      "CONFIG: saved panel brightness p1=%u p2=%u p3=%u p4=%u p5=%u, "
      "global=%u%%\n",
      static_cast<unsigned>(_pendingBrightness.panelPercent[0]),
      static_cast<unsigned>(_pendingBrightness.panelPercent[1]),
      static_cast<unsigned>(_pendingBrightness.panelPercent[2]),
      static_cast<unsigned>(_pendingBrightness.panelPercent[3]),
      static_cast<unsigned>(_pendingBrightness.panelPercent[4]),
      static_cast<unsigned>(_pendingBrightness.globalPercent));
}

void FlowersNetworkManager::checkWatchdog() {
  if (WiFi.status() == WL_CONNECTED && _mqtt.connected()) {
    _lastConnectionMs = millis();
    return;
  }

  if (millis() - _lastConnectionMs > cfg::kConnectionWatchdogMs) {
    Serial.printf(
        "NET: watchdog timeout (%lu s without Wi-Fi/MQTT), restarting\n",
        cfg::kConnectionWatchdogMs / 1000UL);
    flushBrightnessSave(true);
    delay(1000);
    ESP.restart();
  }
}

void FlowersNetworkManager::publishGroupStates(bool force) {
  if (!_mqtt.connected() || _panels == nullptr) {
    return;
  }

  bool bottom = false;
  bool top = false;
  bool full = false;

  for (uint8_t i = 0; i < _panelCount; ++i) {
    if (!_panels[i].isOn()) {
      continue;
    }
    full = true;
    if (i < 2) {
      bottom = true;
    } else {
      top = true;
    }
  }

  const bool publishAll = force || _firstRunStatus;

  if (publishAll || bottom != _lastGroupBottom) {
    _mqtt.publish(cfg::kTopicBottomLightState, bottom ? "ON" : "OFF", true);
    _lastGroupBottom = bottom;
  }
  if (publishAll || top != _lastGroupTop) {
    _mqtt.publish(cfg::kTopicTopLightState, top ? "ON" : "OFF", true);
    _lastGroupTop = top;
  }
  if (publishAll || full != _lastGroupFull) {
    _mqtt.publish(cfg::kTopicFullLightState, full ? "ON" : "OFF", true);
    _lastGroupFull = full;
  }

  _firstRunStatus = false;
}

void FlowersNetworkManager::publishStatus() {
  if (!_mqtt.connected() || _panels == nullptr) {
    return;
  }

  JsonDocument doc;

  JsonObject system = doc["system"].to<JsonObject>();
  system["free_ram"] = ESP.getFreeHeap();
  system["uptime"] = millis() / 1000;
  system["reset_reason"] = resetReasonName(esp_reset_reason());
  system["rssi"] = WiFi.RSSI();
  // The running firmware's standard uses "ACTIVE" while sunrise is running.
  system["sunrise_mode"] = _sunriseActive ? "ACTIVE" : "OFF";
  system["sunset_mode"] = _sunsetActive ? "ACTIVE" : "OFF";

  JsonObject configBlock = doc["config"].to<JsonObject>();
  configBlock["fade_s"] = config.fadeTimeSec();
  configBlock["int_s"] = config.waveIntervalSec();

  doc["brightness"] = _globalBrightnessPercent;

  // Each panel carries both its logical state and its own brightness
  // setpoint. The setpoint is kept while the panel is off, which is exactly
  // what the next ON restores; it is not the instantaneous fade/PWM level.
  JsonObject panelsBlock = doc["panels"].to<JsonObject>();
  for (uint8_t i = 0; i < _panelCount; ++i) {
    char key[3] = {'p', static_cast<char>('1' + i), '\0'};
    JsonObject panelBlock = panelsBlock[key].to<JsonObject>();
    panelBlock["state"] = _panels[i].isOn() ? "ON" : "OFF";
    panelBlock["brightness"] =
        static_cast<int>(std::lroundf(_panels[i].brightnessPercent()));
  }

  String payload;
  serializeJson(doc, payload);
  _mqtt.publish(cfg::kTopicStatus, payload.c_str(), false);
}

void FlowersNetworkManager::publishAllStates() {
  if (!_mqtt.connected() || _panels == nullptr) {
    return;
  }

  for (uint8_t i = 0; i < _panelCount; ++i) {
    publishPanelState(_panels[i]);
    publishPanelBrightness(_panels[i]);
  }
  publishGlobalBrightness();
  publishSunriseState();
  publishSunsetState();
  publishStartWaveState();
  publishGroupStates(true);
  publishStatus();
}

void FlowersNetworkManager::publishPanelState(const LedPanel& panel) {
  if (!_mqtt.connected()) {
    return;
  }

  const String stateTopic = String(cfg::kTopicPanelPrefix) +
                            String(panel.id()) +
                            String(cfg::kTopicPanelStateSuffix);
  const char* payload = panel.isOn() ? "ON" : "OFF";

  _mqtt.publish(stateTopic.c_str(), payload, true);

  // Do NOT mirror the state to panel_N: panel_N is the command topic. When an
  // effect stop published OFF for all panels and a manual ON followed in the
  // same callback, the delayed OFF echo arrived after ON (and vice versa),
  // turning the shelf into a self-sustaining ON/OFF loop.
}

void FlowersNetworkManager::publishPanelBrightness(const LedPanel& panel) {
  if (!_mqtt.connected()) {
    return;
  }

  const String topic = String(cfg::kTopicPanelPrefix) + String(panel.id()) +
                       String(cfg::kTopicPanelBrightnessStateSuffix);
  char payload[8];
  std::snprintf(payload, sizeof(payload), "%d",
                static_cast<int>(std::lroundf(panel.brightnessPercent())));
  _mqtt.publish(topic.c_str(), payload, true);
}

void FlowersNetworkManager::publishGlobalBrightness() {
  if (!_mqtt.connected()) {
    return;
  }

  char payload[8];
  std::snprintf(payload, sizeof(payload), "%d", _globalBrightnessPercent);
  _mqtt.publish(cfg::kTopicBrightnessState, payload, true);
}

void FlowersNetworkManager::publishSunriseState() {
  if (!_mqtt.connected()) {
    return;
  }
  _mqtt.publish(cfg::kTopicSunriseState, _sunriseActive ? "ON" : "OFF", true);
}

void FlowersNetworkManager::publishSunsetState() {
  if (!_mqtt.connected()) {
    return;
  }
  _mqtt.publish(cfg::kTopicSunsetState, _sunsetActive ? "ON" : "OFF", true);
}

void FlowersNetworkManager::publishStartWaveState() {
  if (!_mqtt.connected()) {
    return;
  }
  _mqtt.publish(cfg::kTopicStartWaveState, _startWaveActive ? "ON" : "OFF",
                true);
}

void FlowersNetworkManager::handleMessage(char* topic, uint8_t* payload,
                                          unsigned int length) {
  if (topic == nullptr || _panels == nullptr) {
    return;
  }

  String message;
  if (payload != nullptr) {
    message.reserve(length);
    for (unsigned int i = 0; i < length; ++i) {
      message += static_cast<char>(payload[i]);
    }
  }
  message.trim();

  const String topicStr(topic);
  const String base(cfg::kTopicBase);
  if (!topicStr.startsWith(base)) {
    return;
  }

  const String suffix = topicStr.substring(base.length());

  if (suffix.startsWith("panel_")) {
    unsigned int pos = 6;  // strlen("panel_")
    long panelNumber = 0;
    bool hasDigits = false;

    while (pos < suffix.length() && suffix[pos] >= '0' && suffix[pos] <= '9') {
      panelNumber = panelNumber * 10 + (suffix[pos] - '0');
      ++pos;
      hasDigits = true;
      if (panelNumber > _panelCount) {
        return;  // also prevents integer overflow on hostile topics
      }
    }

    if (!hasDigits || panelNumber < 1) {
      return;
    }

    LedPanel& panel = _panels[panelNumber - 1];
    const String rest = suffix.substring(pos);

    if (rest.length() == 0) {
      handlePanelPower(panel, message);
    } else if (rest == "/brightness") {
      handlePanelBrightness(panel, message);
    }
    // panel_N_state, panel_N/brightness_state and unknown panel subtopics
    // are either our own retained state or unsupported commands.
    return;
  }

  if (suffix == "brightness") {
    handleGlobalBrightness(message);
  } else if (suffix == "config/fadeTime") {
    handleFadeTime(message);
  } else if (suffix == "config/interval") {
    handleInterval(message);
  } else if (suffix == "config/reboot") {
    handleReboot(message);
  } else if (suffix == "fullLight" || suffix == "topLight" ||
             suffix == "bottomLight") {
    if (_effects == nullptr) {
      return;
    }
    bool on = false;
    if (message == "ON") {
      on = true;
    } else if (message == "OFF") {
      on = false;
    } else {
      Serial.printf("MQTT: invalid %s payload '%s'\n", suffix.c_str(),
                    message.c_str());
      return;
    }

    if (suffix == "fullLight") {
      _effects->setFullLight(on);
    } else if (suffix == "topLight") {
      _effects->setTopLight(on);
    } else {
      _effects->setBottomLight(on);
    }
  } else if (suffix == "sunrise") {
    if (_effects == nullptr) {
      return;
    }
    if (message == "ON") {
      if (!_effects->startSunrise()) {
        Serial.println("EFFECTS: sunrise not started");
      }
    } else if (message == "OFF") {
      _effects->stopSunrise();
    } else {
      Serial.printf("MQTT: invalid sunrise payload '%s'\n", message.c_str());
    }
  } else if (suffix == "sunset") {
    if (_effects == nullptr) {
      return;
    }
    if (message == "ON") {
      if (!_effects->startSunset()) {
        Serial.println("EFFECTS: sunset not started");
      }
    } else if (message == "OFF") {
      _effects->stopSunset();
    } else {
      Serial.printf("MQTT: invalid sunset payload '%s'\n", message.c_str());
    }
  } else if (suffix == "startWave") {
    if (_effects == nullptr) {
      return;
    }
    // Empty retained commands must not trigger the effect: a zero-length
    // message is how retained topics are cleared. Any non-empty payload is
    // still accepted for backward compatibility (HA sends "GO").
    if (message.length() == 0) {
      Serial.println("MQTT: empty startWave ignored");
      return;
    }
    if (_effects->startBreathingWave()) {
      Serial.println("EFFECTS: manual breathing wave started");
    } else {
      Serial.println("EFFECTS: breathing wave not started");
    }
  }
}

void FlowersNetworkManager::handlePanelPower(LedPanel& panel,
                                             const String& message) {
  bool requested;
  if (message == "ON") {
    requested = true;
  } else if (message == "OFF") {
    requested = false;
  } else {
    Serial.printf("MQTT: invalid panel payload '%s'\n", message.c_str());
    return;
  }

  // Anti-echo: panel_N also carries retained state, so our own publication
  // comes straight back. Same target state means there is nothing to do.
  if (panel.isOn() == requested) {
    return;
  }

  Serial.printf("MQTT: panel_%u -> %s\n", static_cast<unsigned>(panel.id()),
                requested ? "ON" : "OFF");
  if (_effects != nullptr) {
    _effects->onManualOverrideForPanel(static_cast<uint8_t>(panel.id() - 1));
  }
  panel.setOn(requested, config.fadeTimeSec());
  publishPanelState(panel);
  publishGroupStates();
}

void FlowersNetworkManager::handlePanelBrightness(LedPanel& panel,
                                                  const String& message) {
  long requested = 0;
  if (!parseLong(message, requested) ||
      requested < cfg::kMinBrightnessPercent ||
      requested > cfg::kMaxBrightnessPercent) {
    Serial.printf("MQTT: invalid panel brightness '%s'\n", message.c_str());
    return;
  }

  const int value = static_cast<int>(requested);
  if (static_cast<int>(std::lroundf(panel.brightnessPercent())) == value) {
    return;  // anti-echo / already at this target
  }

  Serial.printf("MQTT: panel_%u brightness -> %d%%\n",
                static_cast<unsigned>(panel.id()), value);
  if (_effects != nullptr) {
    _effects->onManualOverrideForPanel(static_cast<uint8_t>(panel.id() - 1));
  }
  panel.setBrightnessPercent(static_cast<float>(value), config.fadeTimeSec());
  publishPanelBrightness(panel);
  rememberPanelBrightness(static_cast<uint8_t>(panel.id() - 1),
                          static_cast<uint8_t>(value));
}

void FlowersNetworkManager::handleGlobalBrightness(const String& message) {
  long requested = 0;
  if (!parseLong(message, requested) ||
      requested < cfg::kMinBrightnessPercent ||
      requested > cfg::kMaxBrightnessPercent) {
    Serial.printf("MQTT: invalid global brightness '%s'\n", message.c_str());
    return;
  }

  const int value = static_cast<int>(requested);

  bool allMatch = (_globalBrightnessPercent == value);
  for (uint8_t i = 0; i < _panelCount && allMatch; ++i) {
    if (static_cast<int>(std::lroundf(_panels[i].brightnessPercent())) !=
        value) {
      allMatch = false;
    }
  }
  if (allMatch) {
    return;
  }

  Serial.printf("MQTT: global brightness -> %d%%\n", value);
  if (_effects != nullptr) {
    _effects->onManualOverride();
  }
  setGlobalBrightnessPercent(value);
  for (uint8_t i = 0; i < _panelCount; ++i) {
    _panels[i].setBrightnessPercent(static_cast<float>(value),
                                    config.fadeTimeSec());
    // Keep the retained per-panel brightness in sync with the global command.
    publishPanelBrightness(_panels[i]);
  }
  publishGlobalBrightness();
  rememberGlobalBrightness(static_cast<uint8_t>(value));
}

void FlowersNetworkManager::handleFadeTime(const String& message) {
  float value = 0.0f;
  if (!parseFloat(message, value) || !Config::isValidFadeTime(value)) {
    Serial.printf("MQTT: invalid fadeTime '%s'\n", message.c_str());
    return;
  }
  if (std::fabs(value - config.fadeTimeSec()) < 0.001f) {
    return;
  }
  if (!config.setFadeTimeSec(value)) {
    Serial.println("CONFIG: failed to save fadeTime");
    return;
  }

  Serial.printf("CONFIG: fadeTime=%.2f s\n", value);
  publishStatus();
}

void FlowersNetworkManager::handleInterval(const String& message) {
  float value = 0.0f;
  if (!parseFloat(message, value) || !Config::isValidWaveInterval(value)) {
    Serial.printf("MQTT: invalid interval '%s'\n", message.c_str());
    return;
  }
  if (std::fabs(value - config.waveIntervalSec()) < 0.001f) {
    return;
  }
  if (!config.setWaveIntervalSec(value)) {
    Serial.println("CONFIG: failed to save interval");
    return;
  }

  Serial.printf("CONFIG: interval=%.2f s\n", value);
  publishStatus();
}

void FlowersNetworkManager::handleReboot(const String& message) {
  if (message != "ON") {
    return;
  }
  Serial.println("MQTT: reboot requested");
  flushBrightnessSave(true);
  publishStatus();
  delay(500);
  ESP.restart();
}
