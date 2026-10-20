# Graph Report - flowers  (2026-10-20)

## Corpus Check
- 10 files · ~7,030 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 184 nodes · 246 edges · 13 communities (12 shown, 1 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 19 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `a949cf7a`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Config
- docs/main.cpp
- LedPanel
- РАЗДЕЛ 2. Полное Техническое Задание (PRD)
- РАЗДЕЛ 4. Поэтапный план рефакторинга (`write-plans`)
- GlitchManager
- WaveAnimation
- SunriseManager
- Cie1931.h
- README.md
- LedPanel
- LedPanel.cpp

## God Nodes (most connected - your core abstractions)
1. `LedPanel` - 37 edges
2. `LedPanel` - 22 edges
3. `Config` - 15 edges
4. `mqttCallback()` - 10 edges
5. `loop()` - 10 edges
6. `WaveAnimation` - 8 edges
7. `GlitchManager` - 7 edges
8. `РАЗДЕЛ 4. Поэтапный план рефакторинга (`write-plans`)` - 7 edges
9. `updateGlitchManager()` - 6 edges
10. `fadeTo` - 6 edges

## Surprising Connections (you probably didn't know these)
- `LedPanel::cancelBreathing()` --calls--> `fadeTo`  [INFERRED]
  src/LedPanel.cpp → include/LedPanel.h
- `LedPanel::startBreathing()` --calls--> `fadeTo`  [INFERRED]
  src/LedPanel.cpp → include/LedPanel.h
- `Config::resetToDefaults()` --calls--> `persistFloat`  [INFERRED]
  src/Config.cpp → include/Config.h
- `Config::setFadeTimeSec()` --calls--> `persistFloat`  [INFERRED]
  src/Config.cpp → include/Config.h
- `Config::setWaveIntervalSec()` --calls--> `persistFloat`  [INFERRED]
  src/Config.cpp → include/Config.h

## Import Cycles
- None detected.

## Communities (13 total, 1 thin omitted)

### Community 0 - "Config"
Cohesion: 0.11
Nodes (16): Config, begin, persistFloat, _prefs, resetToDefaults, setFadeTimeSec, _settings, setWaveIntervalSec (+8 more)

### Community 1 - "docs/main.cpp"
Cohesion: 0.20
Nodes (19): byte, checkConnectionWatchdog(), loadConfig(), loop(), mqttCallback(), reconnect(), saveConfigFade(), saveConfigInterval() (+11 more)

### Community 2 - "LedPanel"
Cohesion: 0.11
Nodes (14): LedPanel, _breathMode, _breathPhaseDown, _channel, _currentDuty, _fading, _id, _isOn (+6 more)

### Community 3 - "РАЗДЕЛ 2. Полное Техническое Задание (PRD)"
Cohesion: 0.15
Nodes (13): 1. Аппаратная база (Без изменений), 2. Модульная архитектура прошивки, 3. Математика света и ШИМ (CIE1931), 4.1. Подключение, 4.2. Контракт топиков (Индивидуальное, групповое и системное управление), 4.3. Формат телеметрии (`home/flat7/flowers/status`), 4. Сетевой стек и MQTT-протокол, 5.1. Режим «Рассвет» (Sunrise) (+5 more)

### Community 4 - "РАЗДЕЛ 4. Поэтапный план рефакторинга (`write-plans`)"
Cohesion: 0.18
Nodes (10): Блок 1: Аудит `docs/main.cpp`, настройка сборки и выделение `Secrets.h` + `Config`, Блок 2: Модуль `Cie1931` и рефакторинг класса `LedPanel` (Индивидуальная яркость), Блок 3: Модуль `NetworkManager` (Динамический Gateway IP, Watchdog, Телеметрия), Блок 4: Роутинг команд MQTT и индивидуальное управление полками, Блок 5: Модуль `EffectsManager` — Каскадные волны и «Рассвет», Блок 6: Обновленные «Дыхание» и «Глюк» (Без ограничений по яркости) + Финализация, ИНСТРУКЦИЯ ДЛЯ ИИ-АГЕНТА (WORKFLOW), РАЗДЕЛ 1. Brainstorming: инварианты и критические нюансы (+2 more)

### Community 5 - "GlitchManager"
Cohesion: 0.29
Nodes (7): GlitchManager, active, effectEndTime, nextFlickerTime, nextGlitchTime, originalDuty, targetPanelIndex

### Community 6 - "WaveAnimation"
Cohesion: 0.29
Nodes (7): WaveAnimation, active, currentIndex, isBreathingWave, lastStepTime, sequence, turningOn

### Community 7 - "SunriseManager"
Cohesion: 0.50
Nodes (4): SunriseManager, active, panelStarted, startTime

### Community 8 - "Cie1931.h"
Cohesion: 0.29
Nodes (8): clampPercent(), dutyToPercent(), percentToDuty(), percentToLuminance(), initPanels(), printBootInfo(), printCieCheck(), setup()

### Community 10 - "LedPanel"
Cohesion: 0.06
Nodes (23): LedPanel, begin, _breathDurationSec, _breathing, _breathPhaseDown, _brightnessChanged, _channel, consumeBrightnessChanged (+15 more)

### Community 11 - "LedPanel.cpp"
Cohesion: 0.18
Nodes (13): applyPercent, cancelBreathing, fadeTo, writeDuty, LedPanel::applyPercent(), LedPanel::begin(), LedPanel::cancelBreathing(), LedPanel::fadeTo() (+5 more)

## Knowledge Gaps
- **76 isolated node(s):** `_pin`, `_id`, `_channel`, `_currentDuty`, `_targetDuty` (+71 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `LedPanel` connect `LedPanel` to `Config`, `LedPanel.cpp`?**
  _High betweenness centrality (0.469) - this node is a cross-community bridge._
- **Why does `LedPanel` connect `LedPanel` to `docs/main.cpp`?**
  _High betweenness centrality (0.148) - this node is a cross-community bridge._
- **What connects `_pin`, `_id`, `_channel` to the rest of the system?**
  _76 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.11 - nodes in this community are weakly interconnected._
- **Should `LedPanel` be split into smaller, more focused modules?**
  _Cohesion score 0.10822510822510822 - nodes in this community are weakly interconnected._
- **Should `LedPanel` be split into smaller, more focused modules?**
  _Cohesion score 0.06060606060606061 - nodes in this community are weakly interconnected._