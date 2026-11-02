# Graph Report - flowers  (2026-11-02)

## Corpus Check
- 20 files · ~20,897 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 1, .ini 1)

## Summary
- 461 nodes · 658 edges · 19 communities (17 shown, 2 thin omitted)
- Extraction: 85% EXTRACTED · 15% INFERRED · 0% AMBIGUOUS · INFERRED: 100 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `ae5bd318`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Config
- docs/main.cpp
- FlowersNetworkManager
- Wave
- PRD.md
- WaveAnimation
- NetworkManager.cpp
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
- EffectsManager::update
- .stepWave

## God Nodes (most connected - your core abstractions)
1. `FlowersNetworkManager` - 53 edges
2. `EffectsManager` - 44 edges
3. `LedPanel` - 37 edges
4. `LedPanel` - 22 edges
5. `Config` - 18 edges
6. `x18.flowers — умный свет для цветочного стеллажа` - 15 edges
7. `Handoff: x18.flowers` - 14 edges
8. `12. Журнал изменений` - 13 edges
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

## Communities (19 total, 2 thin omitted)

### Community 0 - "Config"
Cohesion: 0.08
Nodes (16): Config, _prefs, _settings, DeviceSettings, fadeTimeSec, waveIntervalSec, resetReasonName(), Config::begin() (+8 more)

### Community 1 - "docs/main.cpp"
Cohesion: 0.08
Nodes (31): checkConnectionWatchdog(), LedPanel, _breathMode, _breathPhaseDown, _channel, _currentDuty, _fading, _id (+23 more)

### Community 2 - "FlowersNetworkManager"
Cohesion: 0.05
Nodes (25): 5.1. `panel_N` — только команда, состояние в `panel_N_state`, 5.2. Остальные топики, 5. Критичные изменения MQTT, EffectsManager, FlowersNetworkManager, _effects, _firstRunStatus, _lastConnectionMs (+17 more)

### Community 3 - "Wave"
Cohesion: 0.29
Nodes (7): Wave, active, count, index, lastStepMs, sequence, turnOn

### Community 4 - "PRD.md"
Cohesion: 0.07
Nodes (29): README, Железо и безопасность, Коммиты, Контекст для продолжения, Порядок работы, Правила работы над проектом, Проверки перед коммитом, x18.flowers — инструкции для Claude Code (+21 more)

### Community 5 - "WaveAnimation"
Cohesion: 0.07
Nodes (26): GlitchManager, active, effectEndTime, nextFlickerTime, nextGlitchTime, originalDuty, targetPanelIndex, SunriseManager (+18 more)

### Community 6 - "NetworkManager.cpp"
Cohesion: 0.08
Nodes (11): FlowersNetworkManager::handleFadeTime(), FlowersNetworkManager::handleGlobalBrightness(), FlowersNetworkManager::handleInterval(), FlowersNetworkManager::handlePanelBrightness(), FlowersNetworkManager::handlePanelPower(), FlowersNetworkManager::handleReboot(), FlowersNetworkManager::publishAllStates(), FlowersNetworkManager::publishPanelBrightness() (+3 more)

### Community 7 - "x18.flowers — умный свет для цветочного стеллажа"
Cohesion: 0.07
Nodes (28): 10. Текущий статус, 11. Правила разработки, 13. Перенос проекта в другую среду, 1. Что это за проект и зачем он нужен, 2. Целевой функционал, 3.1. Состав, 3.2. Распиновка, 3.3. ШИМ (+20 more)

### Community 8 - "12. Журнал изменений"
Cohesion: 0.08
Nodes (17): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), 12. Журнал изменений, 5. Математика света: CIE 1931, Home Assistant — сущности полок и Yandex Smart Home, Блок 1 — фундамент (+9 more)

### Community 9 - "EffectsManager"
Cohesion: 0.07
Nodes (15): EffectsManager, _breathing, _breathingIntervalMs, _breathingLastMs, _glitch, _glitchIntervalMs, _glitchLastMs, _network (+7 more)

### Community 10 - "LedPanel"
Cohesion: 0.05
Nodes (24): LedPanel, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, _fadeStartPercent, _fadeStep (+16 more)

### Community 11 - ".cancelBreathingWave"
Cohesion: 0.25
Nodes (4): EffectsManager::cancelTransientEffects(), EffectsManager::stepBreathingWave(), EffectsManager::updateBreathingWave(), EffectsManager::updateGlitch()

### Community 12 - "EffectsManager.cpp"
Cohesion: 0.14
Nodes (9): EffectsManager::begin(), EffectsManager::cancelBreathingWave(), EffectsManager::finishBreathingWave(), EffectsManager::onManualOverride(), EffectsManager::setBottomLight(), EffectsManager::setFullLight(), EffectsManager::setTopLight(), EffectsManager::startBreathingWave() (+1 more)

### Community 13 - "BreathingWave"
Cohesion: 0.33
Nodes (6): BreathingWave, active, count, index, lastStepMs, sequence

### Community 14 - "РАЗДЕЛ 2. Полное Техническое Задание (PRD)"
Cohesion: 0.10
Nodes (20): 1. Аппаратная база (Без изменений), 2. Модульная архитектура прошивки, 3. Математика света и ШИМ (CIE1931), 4.1. Подключение, 4.2. Контракт топиков (Индивидуальное, групповое и системное управление), 4.3. Формат телеметрии (`home/flat7/flowers/status`), 4. Сетевой стек и MQTT-протокол, 5.1. Режим «Рассвет» (Sunrise) (+12 more)

### Community 15 - "convert_json.js"
Cohesion: 0.24
Nodes (7): config, createdAt, panelParams, panels, system, toInt(), toPanel()

### Community 16 - "Glitch"
Cohesion: 0.33
Nodes (6): Glitch, active, endMs, nextFlickerMs, originalDuty, panelIndex

## Knowledge Gaps
- **86 isolated node(s):** `_pin`, `_id`, `_channel`, `_currentDuty`, `_targetDuty` (+81 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 254 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `EffectsManager` connect `EffectsManager` to `Wave`, `.cancelBreathingWave`, `EffectsManager.cpp`, `BreathingWave`, `Glitch`, `EffectsManager::update`, `.stepWave`?**
  _High betweenness centrality (0.213) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `Config` (e.g. with `Блок 1: Аудит `docs/main.cpp`, настройка сборки и выделение `Secrets.h` + `Config`` and `10. Текущий статус`) actually correct?**
  _`Config` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `_pin`, `_id`, `_channel` to the rest of the system?**
  _86 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.07957957957957958 - nodes in this community are weakly interconnected._
- **Why does `FlowersNetworkManager` connect `FlowersNetworkManager` to `12. Журнал изменений`, `РАЗДЕЛ 2. Полное Техническое Задание (PRD)`, `NetworkManager.cpp`?**
  _High betweenness centrality (0.184) - this node is a cross-community bridge._
- **Should `docs/main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.08282828282828283 - nodes in this community are weakly interconnected._
- **Why does `x18.flowers — умный свет для цветочного стеллажа` connect `x18.flowers — умный свет для цветочного стеллажа` to `12. Журнал изменений`, `PRD.md`, `РАЗДЕЛ 2. Полное Техническое Задание (PRD)`?**
  _High betweenness centrality (0.148) - this node is a cross-community bridge._