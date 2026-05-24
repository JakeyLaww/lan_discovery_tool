CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY
);

CREATE TABLE IF NOT EXISTS devices (
    device_id       TEXT PRIMARY KEY,
    src_ip          TEXT NOT NULL UNIQUE,
    mdns_host       TEXT,
    mac             TEXT,
    status          TEXT NOT NULL DEFAULT 'UNKNOWN',
    first_seen_ms   INTEGER NOT NULL,
    last_seen_ms    INTEGER NOT NULL
);

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

CREATE INDEX IF NOT EXISTS idx_devices_last_seen ON devices(last_seen_ms);
CREATE INDEX IF NOT EXISTS idx_discovery_events_device ON discovery_events(device_id);
CREATE INDEX IF NOT EXISTS idx_discovery_events_received ON discovery_events(received_at_ms);
