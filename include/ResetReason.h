#pragma once

#include <esp_system.h>

// Human-readable reset cause for boot logs and MQTT diagnostics.
// ESP_RST_SW means ESP.restart() (the connection watchdog or an MQTT reboot
// command); BROWNNOUT/PANIC/TASK_WDT point at power or firmware problems.
inline const char* resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:
      return "POWERON";
    case ESP_RST_EXT:
      return "EXT";
    case ESP_RST_SW:
      return "SW";
    case ESP_RST_PANIC:
      return "PANIC";
    case ESP_RST_INT_WDT:
      return "INT_WDT";
    case ESP_RST_TASK_WDT:
      return "TASK_WDT";
    case ESP_RST_WDT:
      return "WDT";
    case ESP_RST_DEEPSLEEP:
      return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:
      return "BROWNOUT";
    default:
      return "UNKNOWN";
  }
}
