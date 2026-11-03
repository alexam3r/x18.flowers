# Graph Report - flowers  (2026-11-03)

## Corpus Check
- 20 files · ~24,729 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 1, .ini 1)

## Summary
- 521 nodes · 752 edges · 26 communities (23 shown, 3 thin omitted)
- Extraction: 83% EXTRACTED · 17% INFERRED · 0% AMBIGUOUS · INFERRED: 125 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `a58cec6a`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Config
- docs/main.cpp
- FlowersNetworkManager
- Wave
- PRD.md
- WaveAnimation
- x18.flowers — умный свет для цветочного стеллажа
- 12. Журнал изменений
- EffectsManager
- LedPanel
- .cancelBreathingWave
- EffectsManager.cpp
- BreathingWave
- РАЗДЕЛ 2. Полное Техническое Задание (PRD)
- convert_json.js
- Glitch
- .resetBreathingSchedule
- FlowersNetworkManager::publishAllStates
- FlowersNetworkManager::update
- FlowersNetworkManager::handleMessage
- LedPanel.cpp
- String
- .cancelSunset
- LedPanel
- .fadeTo

## God Nodes (most connected - your core abstractions)
1. `FlowersNetworkManager` - 63 edges
2. `EffectsManager` - 58 edges
3. `LedPanel` - 39 edges
4. `LedPanel` - 22 edges
5. `Config` - 22 edges
6. `x18.flowers — умный свет для цветочного стеллажа` - 15 edges
7. `12. Журнал изменений` - 15 edges
8. `Handoff: x18.flowers` - 14 edges
9. `loop()` - 12 edges
10. `mqttCallback()` - 10 edges

## Surprising Connections (you probably didn't know these)
- `Блок 1: Аудит `docs/main.cpp`, настройка сборки и выделение `Secrets.h` + `Config`` --references--> `Config`  [INFERRED]
  PRD.md → include/Config.h
- `10. Текущий статус` --references--> `Config`  [INFERRED]
  README.md → include/Config.h
- `Блок 3: Модуль `NetworkManager` (Динамический Gateway IP, Watchdog, Телеметрия)` --references--> `checkConnectionWatchdog()`  [INFERRED]
  PRD.md → docs/main.cpp
- `6.3. Телеметрия` --references--> `Config`  [INFERRED]
  README.md → include/Config.h
- `Контекст для продолжения` --references--> `main()`  [INFERRED]
  AGENTS.md → test/test_cie1931.cpp

## Import Cycles
- None detected.

## Communities (26 total, 3 thin omitted)

### Community 0 - "Config"
Cohesion: 0.06
Nodes (28): Config, _panelBrightness, _prefs, _settings, DeviceSettings, fadeTimeSec, waveIntervalSec, PanelBrightnessSettings (+20 more)

### Community 1 - "docs/main.cpp"
Cohesion: 0.08
Nodes (31): checkConnectionWatchdog(), LedPanel, _breathMode, _breathPhaseDown, _channel, _currentDuty, _fading, _id (+23 more)

### Community 2 - "FlowersNetworkManager"
Cohesion: 0.07
Nodes (20): EffectsManager, FlowersNetworkManager, _brightnessDirty, _brightnessDirtyMs, _effects, _firstRunStatus, _lastConnectionMs, _lastGroupBottom (+12 more)

### Community 3 - "Wave"
Cohesion: 0.29
Nodes (7): Wave, active, count, index, lastStepMs, sequence, turnOn

### Community 4 - "PRD.md"
Cohesion: 0.06
Nodes (33): README, Железо и безопасность, Коммиты, Контекст для продолжения, Порядок работы, Правила работы над проектом, Проверки перед коммитом, x18.flowers — инструкции для Claude Code (+25 more)

### Community 5 - "WaveAnimation"
Cohesion: 0.07
Nodes (26): GlitchManager, active, effectEndTime, nextFlickerTime, nextGlitchTime, originalDuty, targetPanelIndex, SunriseManager (+18 more)

### Community 7 - "x18.flowers — умный свет для цветочного стеллажа"
Cohesion: 0.07
Nodes (30): 10. Текущий статус, 11. Правила разработки, 13. Перенос проекта в другую среду, 1. Что это за проект и зачем он нужен, 2. Целевой функционал, 3.1. Состав, 3.2. Распиновка, 3.3. ШИМ (+22 more)

### Community 8 - "12. Журнал изменений"
Cohesion: 0.07
Nodes (19): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), 12. Журнал изменений, 5. Математика света: CIE 1931, Home Assistant — сущности полок и Yandex Smart Home, Блок 1 — фундамент (+11 more)

### Community 9 - "EffectsManager"
Cohesion: 0.06
Nodes (17): EffectsManager, _breathing, _breathingIntervalMs, _breathingLastMs, _glitch, _glitchIntervalMs, _glitchLastMs, _network (+9 more)

### Community 10 - "LedPanel"
Cohesion: 0.06
Nodes (15): LedPanel, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, _fadeStartPercent, _fadeStep (+7 more)

### Community 11 - ".cancelBreathingWave"
Cohesion: 0.22
Nodes (5): EffectsManager::cancelTransientEffects(), EffectsManager::startBreathingWave(), EffectsManager::stepBreathingWave(), EffectsManager::updateBreathingWave(), EffectsManager::updateGlitch()

### Community 12 - "EffectsManager.cpp"
Cohesion: 0.13
Nodes (8): EffectsManager::onManualOverrideForPanel(), EffectsManager::setBottomLight(), EffectsManager::setTopLight(), EffectsManager::startSunrise(), EffectsManager::startSunset(), EffectsManager::startWave(), EffectsManager::turnAllPanelsOffImmediately(), EffectsManager::updateWave()

### Community 13 - "BreathingWave"
Cohesion: 0.33
Nodes (6): BreathingWave, active, count, index, lastStepMs, sequence

### Community 14 - "РАЗДЕЛ 2. Полное Техническое Задание (PRD)"
Cohesion: 0.08
Nodes (24): 1. Аппаратная база (Без изменений), 2. Модульная архитектура прошивки, 3. Математика света и ШИМ (CIE1931), 4.1. Подключение, 4.2. Контракт топиков (Индивидуальное, групповое и системное управление), 4.3. Формат телеметрии (`home/flat7/flowers/status`), 4.4. Сохранение яркости полок (NVS), 4.5. Режим «Закат» (Sunset) (+16 more)

### Community 15 - "convert_json.js"
Cohesion: 0.24
Nodes (7): config, createdAt, panelParams, panels, system, toInt(), toPanel()

### Community 16 - "Glitch"
Cohesion: 0.33
Nodes (6): Glitch, active, endMs, nextFlickerMs, originalDuty, panelIndex

### Community 17 - ".resetBreathingSchedule"
Cohesion: 0.17
Nodes (4): EffectsManager::begin(), EffectsManager::cancelBreathingWave(), EffectsManager::finishBreathingWave(), EffectsManager::update()

### Community 18 - "FlowersNetworkManager::publishAllStates"
Cohesion: 0.29
Nodes (3): Блок 5 — каскады и «Рассвет», FlowersNetworkManager::handlePanelPower(), FlowersNetworkManager::publishAllStates()

### Community 22 - "LedPanel.cpp"
Cohesion: 0.22
Nodes (7): LedPanel::applyPercent(), LedPanel::begin(), LedPanel::fadeTo(), LedPanel::finishSunset(), LedPanel::setRawPwm(), LedPanel::setSunsetPercent(), LedPanel::update()

### Community 23 - "String"
Cohesion: 0.22
Nodes (7): FlowersNetworkManager::handleFadeTime(), FlowersNetworkManager::handleGlobalBrightness(), FlowersNetworkManager::handleInterval(), FlowersNetworkManager::handlePanelBrightness(), FlowersNetworkManager::handleReboot(), parseFloat(), parseLong()

### Community 24 - ".cancelSunset"
Cohesion: 0.25
Nodes (5): EffectsManager::cancelSunsetPanel(), EffectsManager::onManualOverride(), EffectsManager::setFullLight(), EffectsManager::stopSunset(), EffectsManager::updateSunset()

### Community 25 - "LedPanel"
Cohesion: 0.29
Nodes (3): FlowersNetworkManager::begin(), FlowersNetworkManager::publishPanelBrightness(), FlowersNetworkManager::publishPanelState()

### Community 26 - ".fadeTo"
Cohesion: 0.40
Nodes (4): LedPanel::cancelBreathing(), LedPanel::setBrightnessPercent(), LedPanel::setOn(), LedPanel::startBreathing()

## Knowledge Gaps
- **97 isolated node(s):** `_pin`, `_id`, `_channel`, `_currentDuty`, `_targetDuty` (+92 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 287 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `EffectsManager` connect `EffectsManager` to `Config`, `Wave`, `.cancelBreathingWave`, `EffectsManager.cpp`, `BreathingWave`, `Glitch`, `.resetBreathingSchedule`, `.cancelSunset`?**
  _High betweenness centrality (0.230) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `Config` (e.g. with `Блок 1: Аудит `docs/main.cpp`, настройка сборки и выделение `Secrets.h` + `Config`` and `10. Текущий статус`) actually correct?**
  _`Config` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `_pin`, `_id`, `_channel` to the rest of the system?**
  _97 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.057329462989840346 - nodes in this community are weakly interconnected._
- **Why does `FlowersNetworkManager` connect `FlowersNetworkManager` to `Config`, `PRD.md`, `12. Журнал изменений`, `РАЗДЕЛ 2. Полное Техническое Задание (PRD)`, `FlowersNetworkManager::publishAllStates`, `FlowersNetworkManager::update`, `FlowersNetworkManager::handleMessage`, `String`, `LedPanel`?**
  _High betweenness centrality (0.201) - this node is a cross-community bridge._
- **Should `docs/main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.08282828282828283 - nodes in this community are weakly interconnected._
- **Why does `Config` connect `Config` to `WaveAnimation`, `x18.flowers — умный свет для цветочного стеллажа`?**
  _High betweenness centrality (0.148) - this node is a cross-community bridge._