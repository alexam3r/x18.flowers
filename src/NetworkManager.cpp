#include "NetworkManager.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "EffectsManager.h"
#include "LedPanel.h"
#include "Secrets.h"

namespace {
constexpr unsigned long kMqttRetryIntervalMs = 5000;
constexpr unsigned long kWifiReconnectIntervalMs = 10000;
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

  if (_mqtt.connected()) {
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
  const String subscription = String(cfg::kTopicBase) + "#";
  if (!_mqtt.subscribe(subscription.c_str())) {
    Serial.println("NET: MQTT subscription failed");
  }

  _firstRunStatus = true;
  publishAllStates();
  _lastTelemetryMs = millis();
}

void FlowersNetworkManager::checkWatchdog() {
  if (WiFi.status() == WL_CONNECTED && _mqtt.connected()) {
    _lastConnectionMs = millis();
    return;
  }

  if (millis() - _lastConnectionMs > cfg::kConnectionWatchdogMs) {
    Serial.println("NET: watchdog timeout (180 s without Wi-Fi/MQTT), restarting");
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
  // The running firmware's standard uses "ACTIVE" while sunrise is running.
  system["sunrise_mode"] = _sunriseActive ? "ACTIVE" : "OFF";

  JsonObject configBlock = doc["config"].to<JsonObject>();
  configBlock["fade_s"] = config.fadeTimeSec();
  configBlock["int_s"] = config.waveIntervalSec();

  doc["brightness"] = _globalBrightnessPercent;

  JsonObject panelsBlock = doc["panels"].to<JsonObject>();
  for (uint8_t i = 0; i < _panelCount; ++i) {
    char key[3] = {'p', static_cast<char>('1' + i), '\0'};
    panelsBlock[key] = _panels[i].isOn() ? "ON" : "OFF";
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
  publishGroupStates(true);
  publishStatus();
}

void FlowersNetworkManager::publishPanelState(const LedPanel& panel) {
  if (!_mqtt.connected()) {
    return;
  }

  const String compatTopic =
      String(cfg::kTopicPanelPrefix) + String(panel.id());
  const String stateTopic =
      compatTopic + String(cfg::kTopicPanelStateSuffix);
  const char* payload = panel.isOn() ? "ON" : "OFF";

  _mqtt.publish(stateTopic.c_str(), payload, true);
  // Backward compatibility with the running firmware: panel_N also carries
  // the state. This is exactly why the anti-echo check on panel_N is needed.
  _mqtt.publish(compatTopic.c_str(), payload, true);
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
  } else if (suffix == "startWave") {
    Serial.println("MQTT: startWave will be handled in Block 6");
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
    _effects->onManualOverride();
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
    _effects->onManualOverride();
  }
  panel.setBrightnessPercent(static_cast<float>(value), config.fadeTimeSec());
  publishPanelBrightness(panel);
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
  publishStatus();
  delay(500);
  ESP.restart();
}
