# Graph Report - flowers  (2026-10-20)

## Corpus Check
- 13 files · ~10,708 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 273 nodes · 355 edges · 11 communities
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 29 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `cfcabd89`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Config
- docs/main.cpp
- FlowersNetworkManager
- РАЗДЕЛ 2. Полное Техническое Задание (PRD)
- PRD.md
- GlitchManager
- WaveAnimation
- x18.flowers — умный свет для цветочного стеллажа
- test_cie1931.cpp
- LedPanel
- LedPanel.cpp

## God Nodes (most connected - your core abstractions)
1. `LedPanel` - 37 edges
2. `FlowersNetworkManager` - 33 edges
3. `LedPanel` - 22 edges
4. `Config` - 15 edges
5. `x18.flowers — умный свет для цветочного стеллажа` - 13 edges
6. `mqttCallback()` - 10 edges
7. `loop()` - 10 edges
8. `WaveAnimation` - 8 edges
9. `GlitchManager` - 7 edges
10. `FlowersNetworkManager::update()` - 7 edges

## Surprising Connections (you probably didn't know these)
- `LedPanel::cancelBreathing()` --calls--> `fadeTo`  [INFERRED]
  src/LedPanel.cpp → include/LedPanel.h
- `LedPanel::startBreathing()` --calls--> `fadeTo`  [INFERRED]
  src/LedPanel.cpp → include/LedPanel.h
- `FlowersNetworkManager::update()` --calls--> `maybeReconnectWifi`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h
- `FlowersNetworkManager::update()` --calls--> `tryConnectMqtt`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h
- `FlowersNetworkManager::update()` --calls--> `checkWatchdog`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h

## Import Cycles
- None detected.

## Communities (11 total, 0 thin omitted)

### Community 0 - "Config"
Cohesion: 0.11
Nodes (16): Config, begin, persistFloat, _prefs, resetToDefaults, setFadeTimeSec, _settings, setWaveIntervalSec (+8 more)

### Community 1 - "docs/main.cpp"
Cohesion: 0.08
Nodes (37): byte, checkConnectionWatchdog(), LedPanel, _breathMode, _breathPhaseDown, _channel, _currentDuty, _fading (+29 more)

### Community 2 - "FlowersNetworkManager"
Cohesion: 0.05
Nodes (33): FlowersNetworkManager, begin, checkWatchdog, connectWifi, _firstRunStatus, _lastConnectionMs, _lastGroupBottom, _lastGroupFull (+25 more)

### Community 3 - "РАЗДЕЛ 2. Полное Техническое Задание (PRD)"
Cohesion: 0.15
Nodes (13): 1. Аппаратная база (Без изменений), 2. Модульная архитектура прошивки, 3. Математика света и ШИМ (CIE1931), 4.1. Подключение, 4.2. Контракт топиков (Индивидуальное, групповое и системное управление), 4.3. Формат телеметрии (`home/flat7/flowers/status`), 4. Сетевой стек и MQTT-протокол, 5.1. Режим «Рассвет» (Sunrise) (+5 more)

### Community 4 - "PRD.md"
Cohesion: 0.11
Nodes (16): README, Железо и безопасность, Коммиты, Порядок работы, Правила работы над проектом, Проверки перед коммитом, Блок 1: Аудит `docs/main.cpp`, настройка сборки и выделение `Secrets.h` + `Config`, Блок 2: Модуль `Cie1931` и рефакторинг класса `LedPanel` (Индивидуальная яркость) (+8 more)

### Community 5 - "GlitchManager"
Cohesion: 0.29
Nodes (7): GlitchManager, active, effectEndTime, nextFlickerTime, nextGlitchTime, originalDuty, targetPanelIndex

### Community 6 - "WaveAnimation"
Cohesion: 0.29
Nodes (7): WaveAnimation, active, currentIndex, isBreathingWave, lastStepTime, sequence, turningOn

### Community 7 - "x18.flowers — умный свет для цветочного стеллажа"
Cohesion: 0.06
Nodes (34): 10. Текущий статус, 11. Правила разработки, 12. Журнал изменений, 1. Что это за проект и зачем он нужен, 2. Целевой функционал, 3.1. Состав, 3.2. Распиновка, 3.3. ШИМ (+26 more)

### Community 8 - "test_cie1931.cpp"
Cohesion: 0.12
Nodes (8): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), initPanels(), printBootInfo(), printCieCheck(), setup()

### Community 10 - "LedPanel"
Cohesion: 0.06
Nodes (23): LedPanel, begin, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, consumeBrightnessChanged (+15 more)

### Community 11 - "LedPanel.cpp"
Cohesion: 0.18
Nodes (13): applyPercent, cancelBreathing, fadeTo, writeDuty, LedPanel::applyPercent(), LedPanel::begin(), LedPanel::cancelBreathing(), LedPanel::fadeTo() (+5 more)

## Knowledge Gaps
- **124 isolated node(s):** `_pin`, `_id`, `_channel`, `_currentDuty`, `_targetDuty` (+119 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `LedPanel` connect `LedPanel` to `Config`, `LedPanel.cpp`?**
  _High betweenness centrality (0.212) - this node is a cross-community bridge._
- **What connects `_pin`, `_id`, `_channel` to the rest of the system?**
  _124 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.11 - nodes in this community are weakly interconnected._
- **Should `docs/main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.07535460992907801 - nodes in this community are weakly interconnected._
- **Should `FlowersNetworkManager` be split into smaller, more focused modules?**
  _Cohesion score 0.05357142857142857 - nodes in this community are weakly interconnected._
- **Should `PRD.md` be split into smaller, more focused modules?**
  _Cohesion score 0.1111111111111111 - nodes in this community are weakly interconnected._
- **Should `x18.flowers — умный свет для цветочного стеллажа` be split into smaller, more focused modules?**
  _Cohesion score 0.058823529411764705 - nodes in this community are weakly interconnected._