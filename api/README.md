# LAN Discovery API

FastAPI service that persists scanner discovery events to SQLite.

## Setup

```bash
cd api
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Run locally

```bash
cd api
source .venv/bin/activate
export LAN_DB_PATH=./data/lan.db   # optional, default is api/data/lan.db
uvicorn app.main:app --host 127.0.0.1 --port 8000
```

## Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `LAN_DB_PATH` | `api/data/lan.db` | SQLite database file |
| `LAN_API_TOKEN` | (unset) | If set, require `X-API-Key` header on POST |
| `LAN_HOST` | `127.0.0.1` | Bind host (for reference; pass to uvicorn) |
| `LAN_PORT` | `8000` | Bind port (for reference) |

## Database (dev)

Inspect or wipe the SQLite file without raw `sqlite3`:

```bash
# from repo root
python3 scripts/lan_db.py dump
python3 scripts/lan_db.py reset
```

Honors `LAN_DB_PATH` (same as the API). After `reset`, restart uvicorn; migrations run on startup.

## Endpoints

- `GET /health` — liveness
- `POST /v1/discovery/events` — ingest scanner JSON (202 Accepted)
- `GET /v1/devices` — list devices
- `GET /v1/devices/{device_id}` — device detail

## mDNS host preference

When upserting `devices.mdns_host` from record `host` fields:

1. **SRV** (`type_name == "SRV"`) — overwrites any existing host.
2. **Other types (e.g. A)** — set host only if `mdns_host` is currently empty.

This mirrors the C++ `DeviceRegistry` merge rules.

## Tests

```bash
cd api
source .venv/bin/activate
pytest tests/ -v
```
