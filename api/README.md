# Discovery API

The **Discovery API** (FastAPI) ingests mDNS discovery events from the scanner and persists them to SQLite. The scanner never touches the database directly.

**Build, run, and Docker:** see the [root README](../README.md).

## Endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/health` | Liveness |
| `POST` | `/v1/discovery/events` | Ingest scanner JSON (202 Accepted) |
| `GET` | `/v1/devices` | List devices and services |
| `GET` | `/v1/devices/{device_id}` | Device detail |

## Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `LAN_DB_PATH` | `api/data/lan.db` | SQLite database file |
| `LAN_API_TOKEN` | (unset) | If set, require `X-API-Key` on POST |
| `LAN_HOST` | `127.0.0.1` | Bind host (pass to uvicorn) |
| `LAN_PORT` | `8000` | Bind port (pass to uvicorn) |

## mDNS host preference

When upserting `devices.mdns_host` from record `host` fields:

1. **SRV** (`type_name == "SRV"`) — overwrites any existing host.
2. **Other types (e.g. A)** — set host only if `mdns_host` is currently empty.

This mirrors the C++ `DeviceRegistry` merge rules.

## Database (development)

From the repository root:

```bash
python3 scripts/lan_db.py dump
python3 scripts/lan_db.py reset
```

Honors `LAN_DB_PATH`. After `reset`, restart uvicorn; migrations run on startup.

## Tests

```bash
cd api && source .venv/bin/activate && pytest tests/ -v
```
