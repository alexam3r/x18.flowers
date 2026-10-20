-- x18.flowers: схема телеметрии для MySQL 8.0+ / MariaDB 10.6+.
--
-- Выполнить в базе, к которой подключается Node-RED (например, `USE flowers;`).
-- Время хранится в UTC: Node-RED передаёт created_at в UTC, поэтому EVENT
-- очистки и запросы не зависят от часового пояса сессии MySQL.
--
-- ВАЖНО: если таблица flowers_status уже создана старой версией
-- (колонки uptime и p1..p5-строки), CREATE TABLE IF NOT EXISTS её не изменит.
-- Сохраните старую таблицу и создайте новую:
--   RENAME TABLE flowers_status TO flowers_status_v1;
-- Затем выполните этот файл. Если старые данные не нужны, таблицу можно
-- удалить вместо переименования.
--
-- Очистка старых записей выполняется EVENT-ом, а не DELETE в каждом INSERT:
-- один DELETE на каждую телеметрию (раз в 10 с) создаёт лишнюю нагрузку и
-- конкурирует с вставками. EVENT раз в час делает то же самое дешевле и
-- продолжает работать, даже если Node-RED временно остановлен.

CREATE TABLE IF NOT EXISTS flowers_status (
  id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  created_at      DATETIME(3)     NOT NULL COMMENT 'UTC, время замера из Node-RED',
  free_ram        INT UNSIGNED    NOT NULL COMMENT 'ESP.getFreeHeap(), байт',
  uptime_s        INT UNSIGNED    NOT NULL COMMENT 'uptime, секунды',
  sunrise_mode    VARCHAR(16)     NOT NULL DEFAULT 'OFF' COMMENT 'OFF / ACTIVE',
  fade_s          DECIMAL(4,2)    NOT NULL COMMENT 'fadeTime, секунды',
  int_s           DECIMAL(4,2)    NOT NULL COMMENT 'interval каскада, секунды',
  brightness      TINYINT UNSIGNED NOT NULL COMMENT 'глобальная яркость, %',
  p1_on           TINYINT(1)      NOT NULL COMMENT '1 = ON, 0 = OFF',
  p1_brightness   TINYINT UNSIGNED NOT NULL COMMENT 'яркость полки 1, %',
  p2_on           TINYINT(1)      NOT NULL COMMENT '1 = ON, 0 = OFF',
  p2_brightness   TINYINT UNSIGNED NOT NULL COMMENT 'яркость полки 2, %',
  p3_on           TINYINT(1)      NOT NULL COMMENT '1 = ON, 0 = OFF',
  p3_brightness   TINYINT UNSIGNED NOT NULL COMMENT 'яркость полки 3, %',
  p4_on           TINYINT(1)      NOT NULL COMMENT '1 = ON, 0 = OFF',
  p4_brightness   TINYINT UNSIGNED NOT NULL COMMENT 'яркость полки 4, %',
  p5_on           TINYINT(1)      NOT NULL COMMENT '1 = ON, 0 = OFF',
  p5_brightness   TINYINT UNSIGNED NOT NULL COMMENT 'яркость полки 5, %',
  PRIMARY KEY (id),
  KEY idx_created_at (created_at)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci
  COMMENT='Телеметрия x18.flowers, хранение 72 часа';

-- Проверить планировщик событий:
--   SHOW VARIABLES LIKE 'event_scheduler';
-- Если OFF, включить (нужны права SUPER / SYSTEM_VARIABLES_ADMIN):
--   SET GLOBAL event_scheduler = ON;
-- Для постоянного включения после перезапуска сервера добавить в my.cnf:
--   [mysqld]
--   event_scheduler=ON

CREATE EVENT IF NOT EXISTS ev_flowers_status_cleanup
  ON SCHEDULE EVERY 1 HOUR
  COMMENT 'Удаляет телеметрию x18.flowers старше 72 часов'
  DO
    DELETE FROM flowers_status
    WHERE created_at < UTC_TIMESTAMP(3) - INTERVAL 72 HOUR;

-- Полезные проверки после импорта:
--   SELECT COUNT(*) FROM flowers_status;
--   SELECT * FROM flowers_status ORDER BY created_at DESC LIMIT 10;
--   SHOW CREATE EVENT ev_flowers_status_cleanup;
