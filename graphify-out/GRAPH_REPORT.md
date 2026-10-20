# Graph Report - flowers  (2026-10-20)

## Corpus Check
- 19 files · ~19,740 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 442 nodes · 591 edges · 17 communities
- Extraction: 87% EXTRACTED · 13% INFERRED · 0% AMBIGUOUS · INFERRED: 75 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `b8e30cf2`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Config
- docs/main.cpp
- FlowersNetworkManager
- Wave
- PRD.md
- GlitchManager
- NetworkManager.cpp
- x18.flowers — умный свет для цветочного стеллажа
- test_cie1931.cpp
- EffectsManager
- LedPanel
- LedPanel.cpp
- EffectsManager.cpp
- Handoff: x18.flowers
- РАЗДЕЛ 2. Полное Техническое Задание (PRD)
- convert_json.js
- 9. Сборка, прошивка, тесты

## God Nodes (most connected - your core abstractions)
1. `FlowersNetworkManager` - 51 edges
2. `EffectsManager` - 44 edges
3. `LedPanel` - 37 edges
4. `LedPanel` - 22 edges
5. `Config` - 15 edges
6. `x18.flowers — умный свет для цветочного стеллажа` - 14 edges
7. `Handoff: x18.flowers` - 13 edges
8. `12. Журнал изменений` - 12 edges
9. `mqttCallback()` - 10 edges
10. `loop()` - 10 edges

## Surprising Connections (you probably didn't know these)
- `EffectsManager::update()` --calls--> `startBreathingWave`  [INFERRED]
  src/EffectsManager.cpp → include/EffectsManager.h
- `EffectsManager::update()` --calls--> `updateWave`  [INFERRED]
  src/EffectsManager.cpp → include/EffectsManager.h
- `EffectsManager::update()` --calls--> `updateBreathingWave`  [INFERRED]
  src/EffectsManager.cpp → include/EffectsManager.h
- `EffectsManager::update()` --calls--> `updateGlitch`  [INFERRED]
  src/EffectsManager.cpp → include/EffectsManager.h
- `EffectsManager::onManualOverride()` --calls--> `cancelTransientEffects`  [INFERRED]
  src/EffectsManager.cpp → include/EffectsManager.h

## Import Cycles
- None detected.

## Communities (17 total, 0 thin omitted)

### Community 0 - "Config"
Cohesion: 0.11
Nodes (16): Config, begin, persistFloat, _prefs, resetToDefaults, setFadeTimeSec, _settings, setWaveIntervalSec (+8 more)

### Community 1 - "docs/main.cpp"
Cohesion: 0.06
Nodes (44): byte, checkConnectionWatchdog(), LedPanel, _breathMode, _breathPhaseDown, _channel, _currentDuty, _fading (+36 more)

### Community 2 - "FlowersNetworkManager"
Cohesion: 0.06
Nodes (38): EffectsManager, FlowersNetworkManager, begin, checkWatchdog, connectWifi, _effects, _firstRunStatus, handleFadeTime (+30 more)

### Community 3 - "Wave"
Cohesion: 0.29
Nodes (7): Wave, active, count, index, lastStepMs, sequence, turnOn

### Community 4 - "PRD.md"
Cohesion: 0.09
Nodes (23): README, Железо и безопасность, Коммиты, Контекст для продолжения, Порядок работы, Правила работы над проектом, Проверки перед коммитом, x18.flowers — инструкции для Claude Code (+15 more)

### Community 5 - "GlitchManager"
Cohesion: 0.29
Nodes (7): GlitchManager, active, effectEndTime, nextFlickerTime, nextGlitchTime, originalDuty, targetPanelIndex

### Community 6 - "NetworkManager.cpp"
Cohesion: 0.09
Nodes (23): publishAllStates, publishGlobalBrightness, publishGroupStates, publishPanelBrightness, publishPanelState, publishStartWaveState, publishStatus, publishSunriseState (+15 more)

### Community 7 - "x18.flowers — умный свет для цветочного стеллажа"
Cohesion: 0.05
Nodes (40): 10. Текущий статус, 11. Правила разработки, 12. Журнал изменений, 13. Перенос проекта в другую среду, 1. Что это за проект и зачем он нужен, 2. Целевой функционал, 3.1. Состав, 3.2. Распиновка (+32 more)

### Community 8 - "test_cie1931.cpp"
Cohesion: 0.12
Nodes (8): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), initPanels(), printBootInfo(), printCieCheck(), setup()

### Community 9 - "EffectsManager"
Cohesion: 0.05
Nodes (43): BreathingWave, active, count, index, lastStepMs, sequence, EffectsManager, begin (+35 more)

### Community 10 - "LedPanel"
Cohesion: 0.06
Nodes (23): LedPanel, begin, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, consumeBrightnessChanged (+15 more)

### Community 11 - "LedPanel.cpp"
Cohesion: 0.18
Nodes (13): applyPercent, cancelBreathing, fadeTo, writeDuty, LedPanel::applyPercent(), LedPanel::begin(), LedPanel::cancelBreathing(), LedPanel::fadeTo() (+5 more)

### Community 12 - "EffectsManager.cpp"
Cohesion: 0.09
Nodes (24): FlowersNetworkManager, cancelBreathingWave, cancelGlitch, cancelTransientEffects, finishBreathingWave, resetBreathingSchedule, stepBreathingWave, stepWave (+16 more)

### Community 13 - "Handoff: x18.flowers"
Cohesion: 0.13
Nodes (15): 10. Известные ограничения и возможные улучшения, 11. Правила работы, 12. Готовый промт для нового агента, 1. Что это за проект, 2. Быстрый старт в новой среде, 3. Что лежит в GitHub, 4. Архитектура, 5.1. `panel_N` — только команда, состояние в `panel_N_state` (+7 more)

### Community 14 - "РАЗДЕЛ 2. Полное Техническое Задание (PRD)"
Cohesion: 0.15
Nodes (13): 1. Аппаратная база (Без изменений), 2. Модульная архитектура прошивки, 3. Математика света и ШИМ (CIE1931), 4.1. Подключение, 4.2. Контракт топиков (Индивидуальное, групповое и системное управление), 4.3. Формат телеметрии (`home/flat7/flowers/status`), 4. Сетевой стек и MQTT-протокол, 5.1. Режим «Рассвет» (Sunrise) (+5 more)

### Community 15 - "convert_json.js"
Cohesion: 0.24
Nodes (7): config, createdAt, panelParams, panels, system, toInt(), toPanel()

### Community 16 - "9. Сборка, прошивка, тесты"
Cohesion: 0.22
Nodes (9): 9.1. Требования, 9.2. Секреты, 9.3. Сборка и прошивка, 9.4. Тесты, 9.5. Граф зависимостей, 9.6. Home Assistant, 9.7. Телеметрия в MySQL через Node-RED, 9. Сборка, прошивка, тесты (+1 more)

## Knowledge Gaps
- **200 isolated node(s):** `system`, `config`, `panels`, `createdAt`, `_pin` (+195 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `EffectsManager` connect `EffectsManager` to `Wave`, `EffectsManager.cpp`?**
  _High betweenness centrality (0.235) - this node is a cross-community bridge._
- **Why does `FlowersNetworkManager` connect `FlowersNetworkManager` to `NetworkManager.cpp`?**
  _High betweenness centrality (0.214) - this node is a cross-community bridge._
- **Why does `LedPanel` connect `LedPanel` to `Config`, `LedPanel.cpp`?**
  _High betweenness centrality (0.202) - this node is a cross-community bridge._
- **What connects `system`, `config`, `panels` to the rest of the system?**
  _200 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.11 - nodes in this community are weakly interconnected._
- **Should `docs/main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06262626262626263 - nodes in this community are weakly interconnected._
- **Should `FlowersNetworkManager` be split into smaller, more focused modules?**
  _Cohesion score 0.05990338164251208 - nodes in this community are weakly interconnected._