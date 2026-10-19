/**
 * ============================================================================
 * ПРОЕКТ: УМНЫЙ ЦВЕТОЧНЫЙ СТЕЛЛАЖ - WAVE FEEDBACK EDITION
 * ПЛАТФОРМА: ESP32-C3 SuperMini
 * 
 * ИЗМЕНЕНИЯ В ЭТОЙ ВЕРСИИ:
 * 1. Добавлена обратная связь для эффекта "Волна" (Дыхание).
 *    - При старте волны (ручной или авто) в топик .../startWave_state улетает ON.
 *    - При завершении волны в тот же топик улетает OFF.
 * 
 * ФУНКЦИОНАЛ:
 * - Плавный свет, Рассвет, Дыхание, Помехи, Волны.
 * - Полная синхронизация статусов с Умным домом.
 * ============================================================================
 */

// --- 1. ПОДКЛЮЧЕНИЕ БИБЛИОТЕК ---
#include <Arduino.h>        // База Arduino
#include <WiFi.h>           // Wi-Fi
#include <PubSubClient.h>   // MQTT
#include <ArduinoJson.h>    // JSON
#include <vector>           // Динамические массивы
#include <HardwareSerial.h> // Консоль
#include <Preferences.h>    // Память настроек

// ============================================================================
// 2. НАСТРОЙКИ СЕТИ
// ============================================================================
// ВНИМАНИЕ: реальные учётные данные удалены из референса.
// Актуальные значения хранятся в include/Secrets.h (файл в .gitignore).
const char* WIFI_SSID = "CHANGE_ME";
const char* WIFI_PASS = "CHANGE_ME";

const char* MQTT_SERVER = "10.0.1.1";  // в новой прошивке: WiFi.gatewayIP()
const int MQTT_PORT = 1883;
const char* MQTT_USER = "CHANGE_ME";
const char* MQTT_PASS = "CHANGE_ME";

// ============================================================================
// 3. WATCHDOG (ЗАЩИТА ОТ ЗАВИСАНИЯ)
// ============================================================================
const unsigned long WATCHDOG_TIMEOUT = 180000; // 3 минуты
unsigned long lastConnectionTime = 0; 

// ============================================================================
// 4. ТОПИКИ MQTT (АДРЕСА)
// ============================================================================
const char* TOPIC_BASE = "home/flat7/flowers/";

// --- COMMAND TOPICS (Команды) ---
String TOPIC_BRIGHTNESS = String(TOPIC_BASE) + "brightness";     
String TOPIC_FULL       = String(TOPIC_BASE) + "fullLight";      
String TOPIC_TOP        = String(TOPIC_BASE) + "topLight";       
String TOPIC_BOTTOM     = String(TOPIC_BASE) + "bottomLight";    
String TOPIC_SUNRISE    = String(TOPIC_BASE) + "sunrise";         
String TOPIC_START_WAVE = String(TOPIC_BASE) + "startWave";

// --- STATE TOPICS (Статусы) ---
String TOPIC_STATUS        = String(TOPIC_BASE) + "status"; // JSON телеметрия

// Статусы устройств
String TOPIC_FULL_STATE    = String(TOPIC_BASE) + "fullLight_state";
String TOPIC_TOP_STATE     = String(TOPIC_BASE) + "topLight_state";
String TOPIC_BOTTOM_STATE  = String(TOPIC_BASE) + "bottomLight_state";
String TOPIC_SUNRISE_STATE = String(TOPIC_BASE) + "sunrise_state";
// НОВЫЙ ТОПИК: Статус работы волны
String TOPIC_START_WAVE_STATE = String(TOPIC_BASE) + "startWave_state";

// Топики конфигурации
String TOPIC_CONF_FADE     = String(TOPIC_BASE) + "config/fadeTime"; 
String TOPIC_CONF_INTERVAL = String(TOPIC_BASE) + "config/interval"; 
String TOPIC_CONF_REBOOT   = String(TOPIC_BASE) + "config/reboot";   

// ============================================================================
// 5. ОБОРУДОВАНИЕ (ПИНЫ)
// ============================================================================
#ifdef BOARD_TYPE_C3
    // C3 SuperMini: {Низ ... Верх}
    const int PANEL_PINS[] = {0, 1, 3, 4, 10}; 
#else
    // WROOM
    const int PANEL_PINS[] = {16, 17, 18, 19, 21};
#endif

const int NUM_PANELS = 5; 

// ============================================================================
// 6. НАСТРОЙКИ ШИМ И КОНФИГУРАЦИЯ
// ============================================================================
const int PWM_FREQ = 4000;      
const int PWM_RES = 12;         
const int PWM_MAX = 4095;       

float cfgFadeTimeSec = 2.0;     
float cfgWaveIntervalSec = 0.5; 
const int FADE_STEPS = 200;      

const float SUNRISE_FADE_DURATION = 360.0;        
const unsigned long SUNRISE_INTERVAL_MS = 120000; 

// ============================================================================
// 7. ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
// ============================================================================
WiFiClient espClient;           
PubSubClient client(espClient); 
Preferences preferences;        
String clientId;                

int globalBrightnessPercent = 100;   
unsigned long lastTelemetryTime = 0; 

// Переменные для отслеживания состояния групп
bool lastGroupStateFull = false;
bool lastGroupStateTop = false;
bool lastGroupStateBottom = false;
bool firstRunStatus = true; 

// ============================================================================
// 8. КЛАСС ПАНЕЛИ
// ============================================================================
class LedPanel {
private:
    int _pin;           
    int _id;            
    int _channel;       
    
    uint32_t _currentDuty; 
    uint32_t _targetDuty;  
    bool _isOn;            
    
    unsigned long _lastFadeTime; 
    bool _fading;                
    int _stepDelayMs;            
    
    bool _breathMode;       
    bool _breathPhaseDown;  

public:
    LedPanel(int pin, int id, int channel) {
        _pin = pin;
        _id = id;
        _channel = channel;
        _currentDuty = 0;
        _targetDuty = 0;
        _isOn = false;
        _fading = false;
        _breathMode = false;
        _stepDelayMs = 10;
    }

    void begin() {
        ledcSetup(_channel, PWM_FREQ, PWM_RES); 
        ledcAttachPin(_pin, _channel);          
        ledcWrite(_channel, 0);                 
    }

    uint32_t calculateTarget(bool state) {
        if (!state) return 0; 
        return (uint32_t)((globalBrightnessPercent / 100.0) * PWM_MAX);
    }

    void setTargetState(bool state, float durationSec = -1.0, bool forceImmediate = false) {
        if (!forceImmediate && _breathMode) _breathMode = false;

        uint32_t newTarget = calculateTarget(state);
        
        if (!_breathMode && abs((long)_currentDuty - (long)newTarget) < 50) return;

        _targetDuty = newTarget;
        _isOn = state; 
        _fading = true; 
        _lastFadeTime = millis();

        float timeToUse = (durationSec < 0) ? cfgFadeTimeSec : durationSec;
        if (timeToUse < 0.1) timeToUse = 0.1; 
        
        _stepDelayMs = (int)((timeToUse * 1000.0) / FADE_STEPS);
        if (_stepDelayMs < 1) _stepDelayMs = 1;
    }

    void triggerBreath() {
        if (!_isOn) return; 
        _breathMode = true;
        _breathPhaseDown = true; 
        _targetDuty = 0;         
        _fading = true;
        _stepDelayMs = (int)((2.0 * 1000.0) / FADE_STEPS); 
    }

    bool isOn() { return _isOn; }

    void setRawPwm(uint32_t duty) {
        ledcWrite(_channel, duty);
    }
    
    uint32_t getCurrentDuty() {
        return _currentDuty;
    }

    bool isFading() {
        return _fading;
    }

    void update() {
        if (!_fading) return; 

        if (millis() - _lastFadeTime >= _stepDelayMs) {
            _lastFadeTime = millis();

            long diff = (long)_targetDuty - (long)_currentDuty;
            int minStep = (PWM_MAX / FADE_STEPS); 
            if (minStep < 1) minStep = 1;

            if (abs(diff) <= minStep) {
                _currentDuty = _targetDuty;
                ledcWrite(_channel, _currentDuty); 
                
                if (_breathMode && _breathPhaseDown) {
                    _breathPhaseDown = false; 
                    _targetDuty = calculateTarget(true); 
                } 
                else if (_breathMode && !_breathPhaseDown) {
                    _breathMode = false;
                    _fading = false;
                    publishState(); 
                }
                else {
                    _fading = false;
                    publishState(); 
                }
            } 
            else {
                int step = diff / abs(diff) * minStep;
                if (step == 0) step = (diff > 0) ? 1 : -1;
                _currentDuty += step;               
                ledcWrite(_channel, _currentDuty);  
            }
        }
    }

    void publishState() {
        if (!client.connected()) return; 
        String topic = String(TOPIC_BASE) + "panel_" + String(_id);
        String payload = _isOn ? "ON" : "OFF";
        client.publish(topic.c_str(), payload.c_str(), true);
    }
};

std::vector<LedPanel> panels;

// ============================================================================
// 9. ОБНОВЛЕНИЕ ГРУППОВЫХ СТАТУСОВ
// ============================================================================
void updateGroupStatuses() {
    if (!client.connected()) return;

    // Низ (1, 2)
    bool currentBottom = panels[0].isOn() || panels[1].isOn();
    if (currentBottom != lastGroupStateBottom || firstRunStatus) {
        client.publish(TOPIC_BOTTOM_STATE.c_str(), currentBottom ? "ON" : "OFF", true); 
        lastGroupStateBottom = currentBottom;
    }

    // Верх (3, 4, 5)
    bool currentTop = panels[2].isOn() || panels[3].isOn() || panels[4].isOn();
    if (currentTop != lastGroupStateTop || firstRunStatus) {
        client.publish(TOPIC_TOP_STATE.c_str(), currentTop ? "ON" : "OFF", true); 
        lastGroupStateTop = currentTop;
    }

    // Весь стеллаж
    bool currentFull = currentBottom || currentTop;
    if (currentFull != lastGroupStateFull || firstRunStatus) {
        client.publish(TOPIC_FULL_STATE.c_str(), currentFull ? "ON" : "OFF", true); 
        lastGroupStateFull = currentFull;
    }

    firstRunStatus = false; 
}

// ============================================================================
// 10. МЕНЕДЖЕРЫ ЭФФЕКТОВ
// ============================================================================

// --- РАССВЕТ ---
struct SunriseManager {
    bool active = false;         
    unsigned long startTime = 0; 
    bool panelStarted[5] = {false}; 
} sunriseMgr;

void startSunrise() {
    for (auto &p : panels) { if (p.isOn()) return; }
    Serial.println("SUNRISE: Started!");
    globalBrightnessPercent = 100; 
    sunriseMgr.active = true;
    sunriseMgr.startTime = millis(); 
    for(int i=0; i<5; i++) sunriseMgr.panelStarted[i] = false;
    
    // Статус рассвета
    client.publish(TOPIC_SUNRISE_STATE.c_str(), "ON", true);
}

void stopSunrise() {
    if (!sunriseMgr.active) return; 
    Serial.println("SUNRISE: Stopped.");
    sunriseMgr.active = false;
    for (auto &p : panels) { p.setTargetState(false); }
    
    // Статус рассвета
    client.publish(TOPIC_SUNRISE_STATE.c_str(), "OFF", true);
}

void updateSunrise() {
    if (!sunriseMgr.active) return;
    unsigned long elapsed = millis() - sunriseMgr.startTime;
    for (int i = 0; i < NUM_PANELS; i++) {
        unsigned long startThreshold = i * SUNRISE_INTERVAL_MS;
        if (elapsed >= startThreshold && !sunriseMgr.panelStarted[i]) {
            Serial.printf("SUNRISE: Panel %d go!\n", i+1);
            sunriseMgr.panelStarted[i] = true; 
            panels[i].setTargetState(true, SUNRISE_FADE_DURATION);
        }
    }
    if (elapsed > 15 * 60 * 1000) {
        sunriseMgr.active = false;
        Serial.println("SUNRISE: Done.");
    }
}

// --- ВОЛНЫ (WAVE) ---
struct WaveAnimation {
    bool active = false;        
    bool turningOn = true;      
    std::vector<int> sequence;  
    int currentIndex = 0;       
    unsigned long lastStepTime = 0; 
    bool isBreathingWave = false;   
} waveAnim;

void startWave(std::vector<int> pinIndices, bool turningOn, bool isBreathing = false) {
    if (sunriseMgr.active && !turningOn) { stopSunrise(); return; }
    
    waveAnim.sequence = pinIndices;
    waveAnim.turningOn = turningOn;
    waveAnim.active = true; // Волна активна
    waveAnim.currentIndex = 0;
    waveAnim.lastStepTime = 0;
    waveAnim.isBreathingWave = isBreathing;

    // --- ОТПРАВЛЯЕМ СТАТУС В MQTT ---
    // Сообщаем, что волна запущена
    if (client.connected()) {
        client.publish(TOPIC_START_WAVE_STATE.c_str(), "ON", true);
    }
}

void updateWaveAnimation() {
    if (!waveAnim.active) return;
    
    unsigned long intervalMs = (unsigned long)(cfgWaveIntervalSec * 1000);
    
    if (millis() - waveAnim.lastStepTime > intervalMs) {
        waveAnim.lastStepTime = millis();
        if (waveAnim.currentIndex < waveAnim.sequence.size()) {
            int pIdx = waveAnim.sequence[waveAnim.currentIndex];
            if (waveAnim.isBreathingWave) {
                if (!panels[pIdx].isOn()) { 
                    waveAnim.active = false; 
                    // Если прервали, отправляем OFF
                    client.publish(TOPIC_START_WAVE_STATE.c_str(), "OFF", true);
                    return; 
                }
                panels[pIdx].triggerBreath(); 
            } else {
                panels[pIdx].setTargetState(waveAnim.turningOn, cfgFadeTimeSec);
            }
            waveAnim.currentIndex++; 
        } else {
            // Список закончился, волна завершена
            waveAnim.active = false;
            
            // --- ОТПРАВЛЯЕМ СТАТУС В MQTT ---
            // Сообщаем, что волна остановилась
            if (client.connected()) {
                client.publish(TOPIC_START_WAVE_STATE.c_str(), "OFF", true);
            }
        }
    }
}

// --- ДЫХАНИЕ (BREATHING) ---
unsigned long nextBreathTime = 0;

void setupBreathing() {
    int waitSec = random(480, 600); 
    nextBreathTime = millis() + (waitSec * 1000);
}

void updateBreathingManager() {
    if (millis() >= nextBreathTime) {
        if (sunriseMgr.active || globalBrightnessPercent < 100) { setupBreathing(); return; }
        
        bool allOn = true;
        for (auto &p : panels) { if (!p.isOn()) { allOn = false; break; } }

        if (allOn) {
            Serial.println("Breathing Wave Started");
            bool goUp = random(0, 2);
            std::vector<int> seq;
            for(int i=0; i<NUM_PANELS; i++) seq.push_back(i);
            if (!goUp) std::reverse(seq.begin(), seq.end());
            startWave(seq, true, true); 
        }
        setupBreathing(); 
    }
}

// --- ПОМЕХИ (GLITCH) ---
struct GlitchManager {
    unsigned long nextGlitchTime = 0; 
    bool active = false;              
    int targetPanelIndex = -1;        
    unsigned long effectEndTime = 0;  
    unsigned long nextFlickerTime = 0; 
    uint32_t originalDuty = 0;
} glitchMgr;

void scheduleNextGlitch() {
    unsigned long waitMs = random(180000, 240000);
    glitchMgr.nextGlitchTime = millis() + waitMs;
}

void updateGlitchManager() {
    if (!glitchMgr.active) {
        if (millis() >= glitchMgr.nextGlitchTime) {
            if (globalBrightnessPercent >= 100) { scheduleNextGlitch(); return; }
            if (sunriseMgr.active || waveAnim.active) { scheduleNextGlitch(); return; }

            int pIdx = random(0, NUM_PANELS);
            
            if (panels[pIdx].isOn() && !panels[pIdx].isFading()) {
                glitchMgr.active = true;
                glitchMgr.targetPanelIndex = pIdx;
                glitchMgr.originalDuty = panels[pIdx].getCurrentDuty(); 
                unsigned long duration = random(200, 500);
                glitchMgr.effectEndTime = millis() + duration;
                glitchMgr.nextFlickerTime = millis();
            } else {
                scheduleNextGlitch();
            }
        }
    } 
    else {
        if (millis() >= glitchMgr.effectEndTime) {
            panels[glitchMgr.targetPanelIndex].setRawPwm(glitchMgr.originalDuty);
            glitchMgr.active = false;
            scheduleNextGlitch(); 
        } 
        else {
            if (millis() >= glitchMgr.nextFlickerTime) {
                uint32_t glitchDuty;
                if (random(0, 10) > 5) glitchDuty = 0; 
                else glitchDuty = glitchMgr.originalDuty / random(2, 5); 
                panels[glitchMgr.targetPanelIndex].setRawPwm(glitchDuty);
                glitchMgr.nextFlickerTime = millis() + random(30, 100);
            }
        }
    }
}

// ============================================================================
// 11. СЛУЖЕБНЫЕ (WATCHDOG, CONFIG, TELEMETRY)
// ============================================================================
void checkConnectionWatchdog() {
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
        lastConnectionTime = millis(); 
    } 
    else {
        unsigned long timeSinceLastConnect = millis() - lastConnectionTime;
        if (timeSinceLastConnect > WATCHDOG_TIMEOUT) {
            Serial.println("!!! WATCHDOG: Rebooting...");
            delay(1000);
            ESP.restart(); 
        }
    }
}

void loadConfig() {
    preferences.begin("flowers", false); 
    cfgFadeTimeSec = preferences.getFloat("fadeTime", 2.0);
    cfgWaveIntervalSec = preferences.getFloat("interval", 0.5);
    Serial.printf("CONFIG LOADED: Fade=%.1fs, Interval=%.1fs\n", cfgFadeTimeSec, cfgWaveIntervalSec);
}

void saveConfigFade(float val) {
    cfgFadeTimeSec = val;
    preferences.putFloat("fadeTime", val); 
}

void saveConfigInterval(float val) {
    cfgWaveIntervalSec = val;
    preferences.putFloat("interval", val); 
}

void sendTelemetry() {
    if (millis() - lastTelemetryTime > 10000) { 
        lastTelemetryTime = millis();
        JsonDocument doc; 
        
        JsonObject sys = doc["system"].to<JsonObject>();
        sys["free_ram"] = ESP.getFreeHeap(); 
        sys["uptime"] = millis() / 1000;     
        sys["sunrise_mode"] = sunriseMgr.active ? "ACTIVE" : "OFF";
        
        JsonObject conf = doc["config"].to<JsonObject>();
        conf["fade_s"] = cfgFadeTimeSec;
        conf["int_s"] = cfgWaveIntervalSec;

        doc["brightness"] = globalBrightnessPercent;
        
        JsonObject pObj = doc["panels"].to<JsonObject>();
        for (int i = 0; i < NUM_PANELS; i++) {
            String key = "p" + String(i + 1);
            pObj[key] = panels[i].isOn() ? "ON" : "OFF";
        }

        String output;
        serializeJson(doc, output);
        client.publish(TOPIC_STATUS.c_str(), output.c_str());
    }
}

// ============================================================================
// 12. MQTT CALLBACK
// ============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    Serial.printf("MQTT: %s -> %s\n", topicStr.c_str(), msg.c_str());

    // 1. ПАНЕЛИ
    if (topicStr.indexOf("panel_") >= 0) {
        int pNum = topicStr.substring(topicStr.lastIndexOf('_') + 1).toInt();
        if (pNum >= 1 && pNum <= NUM_PANELS) {
            bool requestedState = (msg == "ON");
            if (panels[pNum - 1].isOn() == requestedState) return; 
            if (sunriseMgr.active) stopSunrise();
            panels[pNum - 1].setTargetState(requestedState, cfgFadeTimeSec);
        }
    }
    // 2. КОНФИГИ
    else if (topicStr == TOPIC_CONF_FADE) {
        float val = msg.toFloat();
        if (val >= 0.1 && val <= 10.0) saveConfigFade(val);
    }
    else if (topicStr == TOPIC_CONF_INTERVAL) {
        float val = msg.toFloat();
        if (val >= 0.0 && val <= 5.0) saveConfigInterval(val);
    }
    else if (topicStr == TOPIC_CONF_REBOOT) {
        if (msg == "ON") {
            client.publish(TOPIC_STATUS.c_str(), "{\"system\":\"REBOOTING...\"}");
            delay(500);    
            ESP.restart(); 
        }
    }
    // 3. УПРАВЛЕНИЕ СВЕТОМ
    else if (topicStr == TOPIC_BRIGHTNESS) {
        int val = msg.toInt();
        if (val >= 1 && val <= 100) {
            globalBrightnessPercent = val;
            for (auto &p : panels) { if (p.isOn()) p.setTargetState(true, cfgFadeTimeSec); }
        }
    }
    // 4. РАССВЕТ
    else if (topicStr == TOPIC_SUNRISE) {
        if (msg == "ON") startSunrise();
        else stopSunrise();
    }
    // 5. РУЧНАЯ ВОЛНА
    else if (topicStr == TOPIC_START_WAVE) {
        if (!sunriseMgr.active && globalBrightnessPercent >= 100) {
            bool allOn = true;
            for (auto &p : panels) { if (!p.isOn()) { allOn = false; break; } }
            if (allOn) {
                std::vector<int> seq;
                for(int i=0; i<NUM_PANELS; i++) seq.push_back(i);
                bool goUp = random(0, 2); 
                if (!goUp) std::reverse(seq.begin(), seq.end());
                
                // Функция startWave теперь сама отправит "ON" в startWave_state
                startWave(seq, true, true);
                setupBreathing();
            }
        }
    }
    // 6. ГРУППЫ (FULL, TOP, BOTTOM)
    else if (topicStr == TOPIC_FULL) {
        if (msg == "OFF") {
            if (sunriseMgr.active) stopSunrise(); 
            std::vector<int> seq = {0, 1, 2, 3, 4};
            std::reverse(seq.begin(), seq.end()); 
            startWave(seq, false, false);
        } 
        else if (msg == "ON") {
            if (!sunriseMgr.active) {
                std::vector<int> seq = {0, 1, 2, 3, 4};
                startWave(seq, true, false);
            }
        }
    }
    else if (topicStr == TOPIC_TOP || topicStr == TOPIC_BOTTOM) {
        if (sunriseMgr.active) stopSunrise(); 
        std::vector<int> seq;
        if (topicStr == TOPIC_TOP) seq = {2, 3, 4}; 
        else seq = {0, 1}; 
        bool state = (msg == "ON");
        if (!state) std::reverse(seq.begin(), seq.end());
        startWave(seq, state, false);
    }
}

// Функция переподключения
void reconnect() {
    if (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
            Serial.println("connected");
            client.subscribe((String(TOPIC_BASE) + "#").c_str());
            firstRunStatus = true; 
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
        }
    }
}

// ============================================================================
// 13. SETUP & LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    
    Serial.println("!!! PAUSE 5 SEC (SAFETY) !!!");
    delay(5000);
    Serial.println("System Starting...");

    loadConfig();

    for (int i = 0; i < NUM_PANELS; i++) {
        panels.emplace_back(PANEL_PINS[i], i + 1, i);
        panels[i].begin();
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); 
    WiFi.setTxPower(WIFI_POWER_19_5dBm); 
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting WiFi");
    
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
        delay(500);
        Serial.print(".");
        tries++;
    }
    Serial.println("\nWiFi check done.");

    clientId = "ESP32Client-" + String(random(0xffff), HEX);
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback); 

    setupBreathing();       
    scheduleNextGlitch();   
    
    lastConnectionTime = millis();
}

void loop() {
    checkConnectionWatchdog();

    if (!client.connected() && WiFi.status() == WL_CONNECTED) {
        reconnect();
    }
    
    client.loop(); 

    for (auto &p : panels) p.update();
    
    updateGroupStatuses(); // Обновление топиков *_state

    updateSunrise();           
    updateWaveAnimation();     
    updateBreathingManager();  
    updateGlitchManager(); 
    
    sendTelemetry();           
}
