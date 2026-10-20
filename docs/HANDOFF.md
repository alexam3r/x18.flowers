# Handoff: x18.flowers

Сжатый контекст проекта для продолжения работы в Claude Code, Codex или другом
агенте. Базовая версия заморожена тегом **`v1.0.0`** после успешной проверки на
железе. Текущий `main` расширяет телеметрию: яркость полок в `status`, MySQL и
Node-RED; новый JSON проверен сборкой, но ещё не на железе.

> Сырой лог переписки в репозиторий не добавляем: в нём встречались реальные
> Wi-Fi/MQTT-пароли. Ниже — очищенная выжимка, которой достаточно для работы.

---

## 1. Что это за проект

Прошивка ESP32-C3 SuperMini для пяти светодиодных полок цветочного стеллажа.
Управление идёт по MQTT из Home Assistant и Яндекс Умного Дома. Пять полок
включаются N-MOSFET-транзисторами, яркость — ШИМ 4 кГц / 12 бит по кривой
CIE1931.

Проект вырос из монолита [`docs/main.cpp`](main.cpp) и переписан в модули по
[`PRD.md`](../PRD.md). Все 6 блоков PRD реализованы и проверены на железе.

Основные возможности:

- индивидуальное включение/выключение и яркость каждой полки;
- глобальная яркость и группы «верх»/«низ»/«весь стеллаж»;
- каскадные волны `fullLight` / `topLight` / `bottomLight`;
- «Рассвет» (14 минут);
- «Дыхание» каждые 8–10 минут и по команде `startWave`;
- Glitch каждые 3–4 минуты с точным восстановлением ШИМ;
- телеметрия `status`, watchdog 180 с, динамический адрес брокера.

---

## 2. Быстрый старт в новой среде

```bash
git clone git@github.com:alexam3r/x18.flowers.git
cd x18.flowers

# Секреты не хранятся в Git
cp include/Secrets.h.example include/Secrets.h
$EDITOR include/Secrets.h     # WIFI_SSID, WIFI_PASS, MQTT_USER, MQTT_PASS

pio run                       # сборка
pio test -e native            # host-тесты CIE1931
pio run -t upload             # прошивка
pio device monitor -b 115200
```

Требуется:

- PlatformIO Core (CLI или IDE);
- платформа `espressif32@8.0.0` — закреплена в `platformio.ini`, PlatformIO
  скачает её сам;
- Arduino Core 3.x (идёт с платформой);
- USB-кабель к ESP32-C3 SuperMini (native USB-CDC).

Если `pio` не в PATH после установки PlatformIO:
`~/.platformio/penv/bin/pio`.

`include/Secrets.h` обязателен для сборки: `NetworkManager.cpp` включает его.
Файл в `.gitignore` и не должен попадать в Git.

---

## 3. Что лежит в GitHub

Отслеживаемые файлы (30 на момент обновления handoff):

```text
.gitignore
AGENTS.md
CLAUDE.md
PRD.md
README.md
docs/flowers.yaml          # HA-пакет MQTT
docs/flowers.sql           # схема MySQL + EVENT retention 72 ч
docs/convert_json.js       # Node-RED: status JSON -> INSERT в MySQL
docs/main.cpp              # старый монолит, очищен от реальных секретов
docs/HANDOFF.md            # этот файл
graphify-out/              # граф архитектуры (json/html/report)
include/
  Cie1931.h
  Config.h
  EffectsManager.h
  LedPanel.h
  NetworkManager.h
  Secrets.h.example
platformio.ini
src/
  Config.cpp
  EffectsManager.cpp
  LedPanel.cpp
  NetworkManager.cpp
  main.cpp
test/test_cie1931.cpp
```

В Git **не** попадают:

- `include/Secrets.h` — реальные учётные данные;
- `.pio/`, `.pio-core/` — артефакты и локальное ядро PlatformIO;
- `.dsh/`, `.remember/` — локальное состояние агента;
- `graphify-out/cache/`, датированные бэкапы graphify.

Проверка перед коммитом:

```bash
git ls-files | grep -i secret   # должен остаться только Secrets.h.example
git grep -n "<пароль>" HEAD     # не должно находить реальные секреты
```

---

## 4. Архитектура

```text
main.cpp
  ├── panels[5] LedPanel
  ├── network FlowersNetworkManager
  └── effects EffectsManager

Config        — константы пинов/ШИМ/топиков + NVS fadeTime/interval
Cie1931       — L* [0..100] -> 12-битный duty (чистый header)
LedPanel      — FSM одной полки: fade, breathing, raw PWM, события
NetworkManager— Wi-Fi, MQTT, watchdog, телеметрия, роутинг, anti-echo
EffectsManager— каскады групп, «Рассвет», «Дыхание», Glitch
```

Важные решения:

- Класс сетевого слоя называется **`FlowersNetworkManager`**: в Arduino Core
  3.x уже есть глобальный `NetworkManager` в библиотеке `Network`.
- `Cie1931.h` не зависит от Arduino и тестируется на хосте.
- Фейдинг интерполирует **L\***, а не duty; ровно 200 шагов.
- Формула CIE1931 даёт для 50 % значение **754** (PRD говорит «≈755» —
  расхождение 1 LSB, тест допускает ±2).
- LEDC: `ledcAttach(pin, 4000, 12)` в Core 3.x, fallback на канальный API
  для Core 2.x.

---

## 5. Критичные изменения MQTT

### 5.1. `panel_N` — только команда, состояние в `panel_N_state`

Коммит `738d43e` убрал дублирование состояния в командный топик `panel_N`.

Почему это было критично: `stopSunrise()` публикует OFF для всех полок, затем
ручная команда/`topLight ON` публикует ON. Отложенные эхо-сообщения OFF/ON
приходили в обратном порядке и запускали бесконечное переключение полки.
Anti-echo не спасал, потому что каждое эхо не совпадало с уже изменившимся
состоянием.

Сейчас:

- `panel_N` — **только CMD** (`ON`/`OFF`);
- `panel_N_state` — STATE (retain);
- `panel_N/brightness` — CMD (`1..100`);
- `panel_N/brightness_state` — STATE (retain).

Старые автоматизации, использовавшие `panel_N` как state, нужно перевести на
`panel_N_state`. HA-пакет в репозитории уже использует правильный топик.

### 5.2. Остальные топики

| Топик | Назначение |
| :--- | :--- |
| `home/flat7/flowers/brightness` | глобальная яркость CMD |
| `home/flat7/flowers/brightness_state` | глобальная яркость STATE |
| `home/flat7/flowers/fullLight` | каскад всего стеллажа CMD |
| `home/flat7/flowers/fullLight_state` | STATE |
| `home/flat7/flowers/topLight` / `topLight_state` | верх (3–5) |
| `home/flat7/flowers/bottomLight` / `bottomLight_state` | низ (1–2) |
| `home/flat7/flowers/sunrise` / `sunrise_state` | «Рассвет» |
| `home/flat7/flowers/startWave` / `startWave_state` | «Дыхание» |
| `home/flat7/flowers/config/fadeTime` | 0.1–10.0 с, NVS |
| `home/flat7/flowers/config/interval` | 0.0–5.0 с, NVS |
| `home/flat7/flowers/config/reboot` | `ON` -> `ESP.restart()` |
| `home/flat7/flowers/status` | JSON-телеметрия каждые 10 с; `panels.pN = {state, brightness}` |

Особенности:

- брокер: `WiFi.gatewayIP():1883`, Client ID `flowers-esp32`;
- `WiFi.setSleep(false)`, `WIFI_POWER_19_5dBm`;
- MQTT-буфер 512 байт, socket timeout 2 с;
- watchdog 180 000 мс;
- состояния `*_state` — retain=true, `status` — retain=false, команды не
  retained;
- при подключении `publishAllStates()` публикует все состояния;
- anti-echo: повтор команды с тем же целевым состоянием/яркостью игнорируется.

---

## 6. Поведение эффектов

- **Каскады**: первая полка сразу, следующие с `interval` из NVS.
  `fullLight ON`: 1→5, OFF: 5→1; `topLight`: 3→4→5 / 5→4→3;
  `bottomLight`: 1→2 / 2→1.
- **Рассвет**: только если все полки выключены и не фейдятся; яркость 100 %;
  полка 1 сразу, 2–5 через 120/240/360/480 с; разгорание 360 с; полный цикл
  840 с; `sunrise_state ON/OFF`. Прерывается `fullLight OFF`, `sunrise OFF`,
  `topLight`/`bottomLight` и ручными командами; при остановке все полки
  плавно выключаются. `fullLight ON` во время рассвета игнорируется.
- **Дыхание**: каждые 8–10 минут (`random(480,600)` с) или по `startWave`;
  условия — все 5 полок ON, не активны рассвет/каскад/glitch; каждая полка
  гаснет до 0 и возвращается к своей яркости; `startWave_state ON` до возврата
  последней полки.
- **Glitch**: каждые 180–240 с; случайная включённая и не фейдящаяся полка;
  любая яркость; 200–500 мс, провалы каждые 30–100 мс в 0 или 1/2–1/4 duty;
  восстановление точного CIE1931-ШИМ.
- Ручные команды/группы/рассвет отменяют временные эффекты с восстановлением
  яркости и ШИМ.

---

## 7. Home Assistant и Яндекс

- HA-пакет: [`docs/flowers.yaml`](flowers.yaml).
- Конфиг проверен перечитыванием в Home Assistant: ошибок валидации нет.
- 6 `light`: весь стеллаж + 5 полок (on/off + brightness 1–100).
- 4 `switch`: верх, низ, рассвет, волна.
- 2 `number`: fadeTime, interval; состояние читается из `status`.
- **В YAML-конфигурации MQTT `light` нельзя задавать
  `supported_color_modes`** — это discovery-only опция. Яркость включается
  наличием `brightness_state_topic`/`brightness_command_topic`.
- «Волна» — momentary switch: и ON, и OFF отправляют `GO`, потому что у
  прошивки нет команды остановки; `startWave_state` сам возвращается в OFF.
- `config/reboot` намеренно не вынесен в HA/Yandex.
- Интеграция Yandex Smart Home прошла успешно; домены `light` и `switch`
  работают. `number` может не экспортироваться.
- Стандартная entities-карточка не показывает слайдер яркости для `light`.
  Нужна tile-карточка с `features: [type: light-brightness]` или `light`-карточка.

---

## 8. Состояние проверки

Базовая версия `v1.0.0` проверена на реальном стеллаже: замечаний нет.
Единственная правка — устранение MQTT-эхо-петли `panel_N` — заняла один проход.
HA-конфиг валиден, интеграция Яндекс Умного Дома успешна. Базовая версия
заморожена тегом **`v1.0.0`**.

Текущая итерация поверх `v1.0.0`:

- `status.panels.pN` стал объектом `{state, brightness}`;
- добавлены [`flowers.sql`](flowers.sql) и обновлённый
  [`convert_json.js`](convert_json.js);
- `pio run` — SUCCESS; `pio test -e native` — 6/6;
- `graphify update .` — 442 узла, 591 ребро, 17 сообществ;
- функция Node-RED проверена на эталонном JSON (строка, Buffer, объект) и на
  старом формате `"ON"`/`"OFF"`;
- SQL-схема подготовлена, но импорт в MySQL в этой среде не выполнялся:
  EVENT требует `event_scheduler=ON`;
- новый JSON на железе **ещё не проверен**: нужна перепрошивка и один цикл
  телеметрии.

Проверка базовой версии:

- `pio run` — SUCCESS; RAM ~11.7 %, Flash ~81.4 %;
- `pio test -e native` — 6/6 тестов CIE1931;
- YAML HA валидируется PyYAML, 6 light / 4 switch / 2 number;
- HA перечитывает конфиг без ошибок, сущности работают;
- Yandex Smart Home: интеграция прошла успешно;
- секреты в Git отсутствуют;
- HIL-приёмка пройдена, замечаний нет.

---

## 9. Чек-лист приёмки на железе (пройден)

Приёмка пройдена; пункты ниже фиксируют проверенный объём.

1. Прошивка, Serial 115200, 5-секундная пауза и логи.
2. Wi-Fi/MQTT: `WiFi.gatewayIP():1883`, Client ID `flowers-esp32`.
3. Подписка на `home/flat7/flowers/#`, отсутствие эха.
4. Индивидуальные ON/OFF и яркость каждой полки.
5. Глобальная яркость, группы верх/низ.
6. `config/fadeTime` и `config/interval`, перезагрузка — значения из NVS.
7. «Рассвет»: 14 минут, прерывание `fullLight OFF`/`sunrise OFF`.
8. «Дыхание» при 30 % и 100 %, ручной `startWave`.
9. Glitch на 100 %, визуально и по MQTT.
10. Watchdog: отключение Wi-Fi/MQTT на 3 минуты.
11. HA: 5 светильников полок, слайдеры яркости, Yandex-синхронизация.
12. Очистка старых retained `panel_N` от предыдущей прошивки — при
    необходимости.

---

## 10. Известные ограничения и возможные улучшения

Базовая версия заморожена; перечисленное ниже не блокирует текущую итерацию и
относится к возможным будущим доработкам.

- HIL-приёмка пройдена; отдельных автоматических тестов для эффектов и сети
  по-прежнему нет.
- Flash ~81 %, запас есть, но крупные новые зависимости надо контролировать.
- Нет LWT/`availability` — при пропадании устройства HA узнаёт об этом только
  по отсутствию обновлений.
- `startWave` — только запуск, без остановки.
- Yandex `number` может не поддерживаться (управление светом работает).
- OTA не реализован.
- Старые retained-сообщения `panel_N` могут один раз примениться после
  перепрошивки; при необходимости очистить их в брокере.

---

## 11. Правила работы

Подробно — [`AGENTS.md`](../AGENTS.md), кратко:

1. Коммиты — **на русском**, с описанием итерации.
2. Любое изменение прошивки сопровождается правкой [`README.md`](../README.md).
3. Перед коммитом: `pio run`, `pio test -e native`; после блока —
   `graphify update .`.
4. После блока — коммит, push и пауза на ревью.
5. Секреты — только в `include/Secrets.h`, в Git их быть не должно.
6. Пины `{0,1,3,4,10}` и ШИМ 4 кГц / 12 бит не менять без перепайки.

Для Claude Code: см. [`CLAUDE.md`](../CLAUDE.md).
Для Codex: см. [`AGENTS.md`](../AGENTS.md).

---

## 12. Готовый промт для нового агента

```text
Проект x18.flowers — прошивка ESP32-C3 для пяти LED-полок.
Прочитай README.md, PRD.md, AGENTS.md и docs/HANDOFF.md.

Статус: базовая версия заморожена тегом v1.0.0 после успешной проверки на
железе. HA-конфиг валиден, интеграция Yandex Smart Home успешна, MQTT-эхо-петля
исправлена одним проходом. Текущий main расширяет телеметрию: в status
добавлена яркость полок, добавлены docs/flowers.sql и docs/convert_json.js.
Новый JSON проверен сборкой, но ещё не на железе.
Не меняй замороженный тег без явного запроса. Если просят доработку — создай
ветку, внеси изменения, проверь на железе.

Сначала:
  cp include/Secrets.h.example include/Secrets.h  # если файла нет
  pio run
  pio test -e native

Не коммить include/Secrets.h, .pio, .pio-core, .dsh, .remember.
Коммиты — на русском; при изменении прошивки обновляй README.md.
После изменения: pio run, pio test -e native, graphify update ., commit, push,
проверка на железе.

Главное, что нельзя сломать:
- пины 0,1,3,4,10; GPIO 2/8/9 не использовать;
- ШИМ 4000 Гц, 12 бит, CIE1931;
- MQTT: panel_N — только команда, panel_N_state — состояние;
- динамический брокер WiFi.gatewayIP(), Client ID flowers-esp32;
- watchdog 180 с; anti-echo.

Ничего делать с проектом не нужно, пока не поступит новая задача.
```
