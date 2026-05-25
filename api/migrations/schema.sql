CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY
);

CREATE TABLE IF NOT EXISTS devices (
    device_id       TEXT PRIMARY KEY,
    src_ip          TEXT NOT NULL,
    mdns_host       TEXT,
    mac             TEXT,
    display_name    TEXT,
    status          TEXT NOT NULL DEFAULT 'UNKNOWN',
    first_seen_ms   INTEGER NOT NULL,
    last_seen_ms    INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_devices_src_ip ON devices(src_ip);
CREATE UNIQUE INDEX IF NOT EXISTS idx_devices_mac ON devices(mac) WHERE mac IS NOT NULL;
CREATE INDEX IF NOT EXISTS idx_devices_last_seen ON devices(last_seen_ms);

CREATE TABLE IF NOT EXISTS services (
    device_id    TEXT NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
    service_key  TEXT NOT NULL,
    PRIMARY KEY (device_id, service_key)
);

CREATE TABLE IF NOT EXISTS discovery_events (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id       TEXT NOT NULL REFERENCES devices(device_id),
    received_at_ms  INTEGER NOT NULL,
    payload_json    TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_discovery_events_device ON discovery_events(device_id);
CREATE INDEX IF NOT EXISTS idx_discovery_events_received ON discovery_events(received_at_ms);

CREATE TABLE IF NOT EXISTS device_baselines (
    device_id       TEXT PRIMARY KEY REFERENCES devices(device_id) ON DELETE CASCADE,
    mdns_host       TEXT,
    services_json   TEXT NOT NULL,
    captured_at_ms  INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS alerts (
    alert_id        TEXT PRIMARY KEY,
    device_id       TEXT NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
    alert_type      TEXT NOT NULL,
    message         TEXT NOT NULL,
    severity        TEXT NOT NULL DEFAULT 'warn',
    observed_at_ms  INTEGER NOT NULL,
    dedupe_key      TEXT NOT NULL UNIQUE,
    resolved_at_ms  INTEGER
);

CREATE INDEX IF NOT EXISTS idx_alerts_observed ON alerts(observed_at_ms DESC);
CREATE INDEX IF NOT EXISTS idx_alerts_device ON alerts(device_id);
