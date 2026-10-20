/**
 * Node-RED function node: status MQTT -> MySQL.
 *
 * Вход:  msg.payload — JSON-строка или уже разобранный объект телеметрии
 *        home/flat7/flowers/status.
 * Выход: msg.topic   — INSERT-запрос;
 *        msg.payload — массив параметров для MySQL-node
 *                      (node-red-node-mysql: query = msg.topic,
 *                       params = msg.payload).
 *
 * Старые записи удаляются EVENT-ом ev_flowers_status_cleanup в MySQL,
 * поэтому DELETE в этой функции намеренно отсутствует.
 *
 * created_at формируется в UTC (ISO 8601 -> DATETIME(3)). Часовой пояс
 * Node-RED на результат не влияет; отображать время нужно с переводом
 * в локальную зону на стороне Grafana/HA.
 */

let data = msg.payload;

// MQTT-in может отдать JSON как Buffer, строку или уже готовый объект.
if (typeof Buffer !== 'undefined' && Buffer.isBuffer(data)) {
    data = data.toString('utf8');
}

if (typeof data === 'string') {
    try {
        data = JSON.parse(data);
    } catch (e) {
        node.error('status: ошибка разбора JSON: ' + e.message, msg);
        return null;
    }
}

if (!data || typeof data !== 'object' || Array.isArray(data)) {
    node.error('status: ожидался JSON-объект, получено: ' + typeof data, msg);
    return null;
}

const asObject = (value) =>
    (value && typeof value === 'object' && !Array.isArray(value)) ? value : {};

// Целое с ограничением диапазона: защищает таблицу от мусора в payload.
const toInt = (value, fallback, min, max) => {
    const number = Number(value);
    if (!Number.isFinite(number)) return fallback;
    return Math.min(max, Math.max(min, Math.round(number)));
};

// Для DECIMAL(4,2): не округляем 1.8 до 2.
const toFloat = (value, fallback, min, max) => {
    const number = Number(value);
    if (!Number.isFinite(number)) return fallback;
    return Number(Math.min(max, Math.max(min, number)).toFixed(2));
};

// Новый формат: {"state": "ON", "brightness": 100}.
// Старый формат "ON"/"OFF" тоже поддерживается: яркость тогда 0.
const toPanel = (raw) => {
    if (raw && typeof raw === 'object' && !Array.isArray(raw)) {
        return {
            on: String(raw.state).toUpperCase() === 'ON' ? 1 : 0,
            brightness: toInt(raw.brightness, 0, 0, 100)
        };
    }
    return {
        on: String(raw).toUpperCase() === 'ON' ? 1 : 0,
        brightness: 0
    };
};

const system = asObject(data.system);
const config = asObject(data.config);
const panels = asObject(data.panels);

const createdAt = new Date().toISOString().slice(0, 23).replace('T', ' ');
const panelParams = [1, 2, 3, 4, 5].flatMap((index) => {
    const panel = toPanel(panels['p' + index]);
    return [panel.on, panel.brightness];
});

const insertQuery = `
    INSERT INTO flowers_status
        (created_at, free_ram, uptime_s, sunrise_mode, fade_s, int_s,
         brightness, p1_on, p1_brightness, p2_on, p2_brightness,
         p3_on, p3_brightness, p4_on, p4_brightness, p5_on, p5_brightness)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
`;

msg.topic = insertQuery;
msg.payload = [
    createdAt,
    toInt(system.free_ram, 0, 0, 4294967295),
    toInt(system.uptime, 0, 0, 4294967295),
    String(system.sunrise_mode || 'OFF').trim().toUpperCase().slice(0, 16),
    toFloat(config.fade_s, 0, 0, 99.99),
    toFloat(config.int_s, 0, 0, 99.99),
    toInt(data.brightness, 0, 0, 100),
    ...panelParams
];

return msg;
