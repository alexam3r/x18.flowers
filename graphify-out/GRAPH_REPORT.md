# Graph Report - flowers  (2026-10-20)

## Corpus Check
- 15 files · ~13,709 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 353 nodes · 476 edges · 13 communities
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 53 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `573c3022`
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
- EffectsManager
- LedPanel
- LedPanel.cpp
- EffectsManager.cpp

## God Nodes (most connected - your core abstractions)
1. `FlowersNetworkManager` - 48 edges
2. `LedPanel` - 37 edges
3. `EffectsManager` - 24 edges
4. `LedPanel` - 22 edges
5. `Config` - 15 edges
6. `x18.flowers — умный свет для цветочного стеллажа` - 13 edges
7. `mqttCallback()` - 10 edges
8. `loop()` - 10 edges
9. `WaveAnimation` - 8 edges
10. `GlitchManager` - 7 edges

## Surprising Connections (you probably didn't know these)
- `LedPanel::cancelBreathing()` --calls--> `fadeTo`  [INFERRED]
  src/LedPanel.cpp → include/LedPanel.h
- `LedPanel::startBreathing()` --calls--> `fadeTo`  [INFERRED]
  src/LedPanel.cpp → include/LedPanel.h
- `FlowersNetworkManager::handleGlobalBrightness()` --calls--> `setGlobalBrightnessPercent`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h
- `FlowersNetworkManager::update()` --calls--> `publishStatus`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h
- `FlowersNetworkManager::update()` --calls--> `publishGroupStates`  [INFERRED]
  src/NetworkManager.cpp → include/NetworkManager.h

## Import Cycles
- None detected.

## Communities (13 total, 0 thin omitted)

### Community 0 - "Config"
Cohesion: 0.11
Nodes (15): Config, begin, persistFloat, _prefs, resetToDefaults, setFadeTimeSec, _settings, setWaveIntervalSec (+7 more)

### Community 1 - "docs/main.cpp"
Cohesion: 0.06
Nodes (44): byte, checkConnectionWatchdog(), LedPanel, _breathMode, _breathPhaseDown, _channel, _currentDuty, _fading (+36 more)

### Community 2 - "FlowersNetworkManager"
Cohesion: 0.06
Nodes (38): EffectsManager, FlowersNetworkManager, begin, checkWatchdog, connectWifi, _effects, _firstRunStatus, handleFadeTime (+30 more)

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
Cohesion: 0.08
Nodes (23): publishAllStates, publishGlobalBrightness, publishGroupStates, publishPanelBrightness, publishPanelState, publishStatus, publishSunriseState, setGlobalBrightnessPercent (+15 more)

### Community 7 - "x18.flowers — умный свет для цветочного стеллажа"
Cohesion: 0.05
Nodes (38): 10. Текущий статус, 11. Правила разработки, 12. Журнал изменений, 1. Что это за проект и зачем он нужен, 2. Целевой функционал, 3.1. Состав, 3.2. Распиновка, 3.3. ШИМ (+30 more)

### Community 8 - "test_cie1931.cpp"
Cohesion: 0.12
Nodes (8): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), initPanels(), printBootInfo(), printCieCheck(), setup()

### Community 9 - "EffectsManager"
Cohesion: 0.07
Nodes (26): EffectsManager, begin, _network, onManualOverride, _panelCount, _panels, setBottomLight, setFullLight (+18 more)

### Community 10 - "LedPanel"
Cohesion: 0.06
Nodes (23): LedPanel, begin, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, consumeBrightnessChanged (+15 more)

### Community 11 - "LedPanel.cpp"
Cohesion: 0.18
Nodes (13): applyPercent, cancelBreathing, fadeTo, writeDuty, LedPanel::applyPercent(), LedPanel::begin(), LedPanel::cancelBreathing(), LedPanel::fadeTo() (+5 more)

### Community 12 - "EffectsManager.cpp"
Cohesion: 0.13
Nodes (8): FlowersNetworkManager, stepWave, updateWave, LedPanel, EffectsManager::begin(), EffectsManager::startWave(), EffectsManager::update(), EffectsManager::updateWave()

## Knowledge Gaps
- **150 isolated node(s):** `_pin`, `_id`, `_channel`, `_currentDuty`, `_targetDuty` (+145 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `LedPanel` connect `LedPanel` to `Config`, `LedPanel.cpp`?**
  _High betweenness centrality (0.268) - this node is a cross-community bridge._
- **Why does `FlowersNetworkManager` connect `FlowersNetworkManager` to `NetworkManager.cpp`?**
  _High betweenness centrality (0.218) - this node is a cross-community bridge._
- **Why does `EffectsManager` connect `EffectsManager` to `EffectsManager.cpp`?**
  _High betweenness centrality (0.175) - this node is a cross-community bridge._
- **What connects `_pin`, `_id`, `_channel` to the rest of the system?**
  _150 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.11255411255411256 - nodes in this community are weakly interconnected._
- **Should `docs/main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06262626262626263 - nodes in this community are weakly interconnected._
- **Should `FlowersNetworkManager` be split into smaller, more focused modules?**
  _Cohesion score 0.06342494714587738 - nodes in this community are weakly interconnected._