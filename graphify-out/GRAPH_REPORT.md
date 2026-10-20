# Graph Report - flowers  (2026-10-20)

## Corpus Check
- 15 files · ~14,859 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 399 nodes · 544 edges · 14 communities
- Extraction: 86% EXTRACTED · 14% INFERRED · 0% AMBIGUOUS · INFERRED: 75 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `151e182f`
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
- src/main.cpp
- EffectsManager
- LedPanel
- LedPanel.cpp
- EffectsManager.cpp
- BreathingWave

## God Nodes (most connected - your core abstractions)
1. `FlowersNetworkManager` - 51 edges
2. `EffectsManager` - 44 edges
3. `LedPanel` - 37 edges
4. `LedPanel` - 22 edges
5. `Config` - 15 edges
6. `x18.flowers — умный свет для цветочного стеллажа` - 13 edges
7. `mqttCallback()` - 10 edges
8. `loop()` - 10 edges
9. `WaveAnimation` - 8 edges
10. `FlowersNetworkManager::publishAllStates()` - 8 edges

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

## Communities (14 total, 0 thin omitted)

### Community 0 - "Config"
Cohesion: 0.11
Nodes (15): Config, begin, persistFloat, _prefs, resetToDefaults, setFadeTimeSec, _settings, setWaveIntervalSec (+7 more)

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
Cohesion: 0.06
Nodes (29): README, Железо и безопасность, Коммиты, Порядок работы, Правила работы над проектом, Проверки перед коммитом, 1. Аппаратная база (Без изменений), 2. Модульная архитектура прошивки (+21 more)

### Community 5 - "GlitchManager"
Cohesion: 0.29
Nodes (7): GlitchManager, active, effectEndTime, nextFlickerTime, nextGlitchTime, originalDuty, targetPanelIndex

### Community 6 - "NetworkManager.cpp"
Cohesion: 0.08
Nodes (24): publishAllStates, publishGlobalBrightness, publishGroupStates, publishPanelBrightness, publishPanelState, publishStartWaveState, publishStatus, publishSunriseState (+16 more)

### Community 7 - "x18.flowers — умный свет для цветочного стеллажа"
Cohesion: 0.05
Nodes (40): 10. Текущий статус, 11. Правила разработки, 12. Журнал изменений, 1. Что это за проект и зачем он нужен, 2. Целевой функционал, 3.1. Состав, 3.2. Распиновка, 3.3. ШИМ (+32 more)

### Community 8 - "src/main.cpp"
Cohesion: 0.53
Nodes (4): initPanels(), printBootInfo(), printCieCheck(), setup()

### Community 9 - "EffectsManager"
Cohesion: 0.05
Nodes (37): EffectsManager, begin, _breathing, _breathingIntervalMs, _breathingLastMs, _glitch, _glitchIntervalMs, _glitchLastMs (+29 more)

### Community 10 - "LedPanel"
Cohesion: 0.06
Nodes (23): LedPanel, begin, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, consumeBrightnessChanged (+15 more)

### Community 11 - "LedPanel.cpp"
Cohesion: 0.08
Nodes (17): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), applyPercent, cancelBreathing, fadeTo, writeDuty (+9 more)

### Community 12 - "EffectsManager.cpp"
Cohesion: 0.09
Nodes (24): FlowersNetworkManager, cancelBreathingWave, cancelGlitch, cancelTransientEffects, finishBreathingWave, resetBreathingSchedule, stepBreathingWave, stepWave (+16 more)

### Community 13 - "BreathingWave"
Cohesion: 0.33
Nodes (6): BreathingWave, active, count, index, lastStepMs, sequence

## Knowledge Gaps
- **169 isolated node(s):** `_pin`, `_id`, `_channel`, `_currentDuty`, `_targetDuty` (+164 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `EffectsManager` connect `EffectsManager` to `Wave`, `EffectsManager.cpp`, `BreathingWave`?**
  _High betweenness centrality (0.288) - this node is a cross-community bridge._
- **Why does `LedPanel` connect `LedPanel` to `Config`, `LedPanel.cpp`?**
  _High betweenness centrality (0.249) - this node is a cross-community bridge._
- **Why does `FlowersNetworkManager` connect `FlowersNetworkManager` to `NetworkManager.cpp`?**
  _High betweenness centrality (0.210) - this node is a cross-community bridge._
- **What connects `_pin`, `_id`, `_channel` to the rest of the system?**
  _169 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.11255411255411256 - nodes in this community are weakly interconnected._
- **Should `docs/main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06262626262626263 - nodes in this community are weakly interconnected._
- **Should `FlowersNetworkManager` be split into smaller, more focused modules?**
  _Cohesion score 0.05990338164251208 - nodes in this community are weakly interconnected._