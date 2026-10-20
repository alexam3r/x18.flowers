# Graph Report - flowers  (2026-10-20)

## Corpus Check
- 13 files · ~12,018 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 300 nodes · 417 edges · 12 communities
- Extraction: 88% EXTRACTED · 12% INFERRED · 0% AMBIGUOUS · INFERRED: 49 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `4a637a37`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Config
- docs/main.cpp
- FlowersNetworkManager
- РАЗДЕЛ 2. Полное Техническое Задание (PRD)
- PRD.md
- GlitchManager
- NetworkManager.cpp
- x18.flowers — умный свет для цветочного стеллажа
- test_cie1931.cpp
- SunriseManager
- LedPanel
- LedPanel.cpp

## God Nodes (most connected - your core abstractions)
1. `FlowersNetworkManager` - 44 edges
2. `LedPanel` - 37 edges
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
- `FlowersNetworkManager::handleGlobalBrightness()` --calls--> `setGlobalBrightnessPercent`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h
- `FlowersNetworkManager::handleFadeTime()` --calls--> `publishStatus`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h
- `FlowersNetworkManager::handleInterval()` --calls--> `publishStatus`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h

## Import Cycles
- None detected.

## Communities (12 total, 0 thin omitted)

### Community 0 - "Config"
Cohesion: 0.10
Nodes (18): Config, begin, persistFloat, _prefs, resetToDefaults, setFadeTimeSec, _settings, setWaveIntervalSec (+10 more)

### Community 1 - "docs/main.cpp"
Cohesion: 0.07
Nodes (40): byte, checkConnectionWatchdog(), LedPanel, _breathMode, _breathPhaseDown, _channel, _currentDuty, _fading (+32 more)

### Community 2 - "FlowersNetworkManager"
Cohesion: 0.07
Nodes (36): FlowersNetworkManager, begin, checkWatchdog, connectWifi, _firstRunStatus, handleFadeTime, handleGlobalBrightness, handleInterval (+28 more)

### Community 3 - "РАЗДЕЛ 2. Полное Техническое Задание (PRD)"
Cohesion: 0.15
Nodes (13): 1. Аппаратная база (Без изменений), 2. Модульная архитектура прошивки, 3. Математика света и ШИМ (CIE1931), 4.1. Подключение, 4.2. Контракт топиков (Индивидуальное, групповое и системное управление), 4.3. Формат телеметрии (`home/flat7/flowers/status`), 4. Сетевой стек и MQTT-протокол, 5.1. Режим «Рассвет» (Sunrise) (+5 more)

### Community 4 - "PRD.md"
Cohesion: 0.11
Nodes (16): README, Железо и безопасность, Коммиты, Порядок работы, Правила работы над проектом, Проверки перед коммитом, Блок 1: Аудит `docs/main.cpp`, настройка сборки и выделение `Secrets.h` + `Config`, Блок 2: Модуль `Cie1931` и рефакторинг класса `LedPanel` (Индивидуальная яркость) (+8 more)

### Community 5 - "GlitchManager"
Cohesion: 0.29
Nodes (7): GlitchManager, active, effectEndTime, nextFlickerTime, nextGlitchTime, originalDuty, targetPanelIndex

### Community 6 - "NetworkManager.cpp"
Cohesion: 0.10
Nodes (19): publishAllStates, publishGlobalBrightness, publishGroupStates, publishPanelBrightness, publishPanelState, publishStatus, setGlobalBrightnessPercent, LedPanel (+11 more)

### Community 7 - "x18.flowers — умный свет для цветочного стеллажа"
Cohesion: 0.06
Nodes (36): 10. Текущий статус, 11. Правила разработки, 12. Журнал изменений, 1. Что это за проект и зачем он нужен, 2. Целевой функционал, 3.1. Состав, 3.2. Распиновка, 3.3. ШИМ (+28 more)

### Community 8 - "test_cie1931.cpp"
Cohesion: 0.12
Nodes (8): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), initPanels(), printBootInfo(), printCieCheck(), setup()

### Community 9 - "SunriseManager"
Cohesion: 0.50
Nodes (4): SunriseManager, active, panelStarted, startTime

### Community 10 - "LedPanel"
Cohesion: 0.06
Nodes (23): LedPanel, begin, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, consumeBrightnessChanged (+15 more)

### Community 11 - "LedPanel.cpp"
Cohesion: 0.18
Nodes (13): applyPercent, cancelBreathing, fadeTo, writeDuty, LedPanel::applyPercent(), LedPanel::begin(), LedPanel::cancelBreathing(), LedPanel::fadeTo() (+5 more)

## Knowledge Gaps
- **125 isolated node(s):** `_pin`, `_id`, `_channel`, `_currentDuty`, `_targetDuty` (+120 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `FlowersNetworkManager` connect `FlowersNetworkManager` to `NetworkManager.cpp`?**
  _High betweenness centrality (0.239) - this node is a cross-community bridge._
- **Why does `LedPanel` connect `LedPanel` to `Config`, `LedPanel.cpp`?**
  _High betweenness centrality (0.184) - this node is a cross-community bridge._
- **What connects `_pin`, `_id`, `_channel` to the rest of the system?**
  _125 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.09971509971509972 - nodes in this community are weakly interconnected._
- **Should `docs/main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06980392156862746 - nodes in this community are weakly interconnected._
- **Should `FlowersNetworkManager` be split into smaller, more focused modules?**
  _Cohesion score 0.06707317073170732 - nodes in this community are weakly interconnected._
- **Should `PRD.md` be split into smaller, more focused modules?**
  _Cohesion score 0.1111111111111111 - nodes in this community are weakly interconnected._