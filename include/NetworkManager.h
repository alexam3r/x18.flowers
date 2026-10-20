#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include "Config.h"

class LedPanel;

// The class is named FlowersNetworkManager because Arduino core 3.x already
// defines a global NetworkManager in its Network library.
//
// Owns the Wi-Fi/MQTT transport, the connection watchdog, telemetry and the
// group (full/top/bottom) state topics.
//
// The broker address is never hard-coded: it is taken from the DHCP default
// gateway (WiFi.gatewayIP()) after every Wi-Fi connection.
class FlowersNetworkManager {
 public:
  FlowersNetworkManager();

  // Connects Wi-Fi (blocking for at most kWifiConnectRetries * 500 ms) and
  // prepares the MQTT client. The panel array is used for state/telemetry.
  bool begin(LedPanel* panels, uint8_t panelCount);

  // Must be called from loop(): reconnects, watchdog, telemetry, group states.
  void update();

  bool wifiConnected() const;
  bool mqttConnected();

  IPAddress brokerIp() const { return _brokerIp; }

  int globalBrightnessPercent() const { return _globalBrightnessPercent; }
  void setGlobalBrightnessPercent(int percent);

  bool sunriseActive() const { return _sunriseActive; }
  void setSunriseActive(bool active) { _sunriseActive = active; }

  // Publishes the JSON telemetry (retain=false) and group states (retain=true).
  void publishStatus();
  void publishGroupStates(bool force = false);

  // Retained state topics. Public so effects can report panel changes later.
  void publishPanelState(const LedPanel& panel);
  void publishPanelBrightness(const LedPanel& panel);
  void publishGlobalBrightness();

 private:
  bool connectWifi();
  void resolveBroker();
  void maybeReconnectWifi();
  void tryConnectMqtt();
  void checkWatchdog();

  void publishAllStates();
  void handleMessage(char* topic, uint8_t* payload, unsigned int length);
  void handlePanelPower(LedPanel& panel, const String& message);
  void handlePanelBrightness(LedPanel& panel, const String& message);
  void handleGlobalBrightness(const String& message);
  void handleFadeTime(const String& message);
  void handleInterval(const String& message);
  void handleReboot(const String& message);

  WiFiClient _wifiClient;
  PubSubClient _mqtt;

  LedPanel* _panels = nullptr;
  uint8_t _panelCount = 0;

  IPAddress _brokerIp;
  bool _wifiConnected = false;
  bool _firstRunStatus = true;

  int _globalBrightnessPercent = cfg::kDefaultBrightnessPercent;
  bool _sunriseActive = false;

  unsigned long _lastMqttAttemptMs = 0;
  unsigned long _lastWifiReconnectMs = 0;
  unsigned long _lastTelemetryMs = 0;
  unsigned long _lastConnectionMs = 0;

  bool _lastGroupFull = false;
  bool _lastGroupTop = false;
  bool _lastGroupBottom = false;
};
