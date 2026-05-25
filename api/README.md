# Discovery API

The **Discovery API** (FastAPI) ingests mDNS discovery events from the scanner and persists them to SQLite. The scanner never touches the database directly.

**Build, run, and Docker:** see the [root README](../README.md).

## Endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/health` | Liveness |
| `POST` | `/v1/discovery/events` | Ingest scanner JSON (202 Accepted) |
| `GET` | `/v1/devices` | List devices (status, MAC, **service types**) |
| `GET` | `/v1/devices/{device_id}` | Device detail |
| `PATCH` | `/v1/devices/{device_id}` | Approve device and capture baseline snapshot |
| `GET` | `/v1/alerts` | List security alerts (`?limit=`, `?alert_type=`) |

## Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `LAN_DB_PATH` | `api/data/lan.db` | SQLite database file |
| `LAN_API_TOKEN` | (unset) | If set, require `X-API-Key` on POST and PATCH |
| `LAN_HOST` | `127.0.0.1` | Bind host (pass to uvicorn) |
| `LAN_PORT` | `8000` | Bind port (pass to uvicorn) |

## Manual test (two terminals)

**Terminal 1 — API:**

```bash
cd api && source .venv/bin/activate
uvicorn app.main:app --host 127.0.0.1 --port 8000
```

**Terminal 2 — scanner** (INFO logs; add `-d` only when debugging):

```bash
./build/scanner --api-url http://127.0.0.1:8000
```

**Inspect and approve** (paste the real `device_id` from the list — do not use angle brackets in bash):

```bash
curl -s http://127.0.0.1:8000/v1/devices | python3 -m json.tool

curl -X PATCH "http://127.0.0.1:8000/v1/devices/6c508f18-2e53-4a12-a927-0fc7659c2ef7" \
  -H 'Content-Type: application/json' \
  -d '{"display_name":"Living Room TV"}'

curl -s http://127.0.0.1:8000/v1/alerts | python3 -m json.tool
python3 scripts/lan_db.py dump
```

Expect `device_baselines` with Bonjour **service types** (e.g. `_airplay._tcp.local`), not instance display names.

## Device identity (Phase 3)

Stable identity resolution order:

1. **MAC** — primary key for “same physical device” across DHCP IP changes
2. **mDNS hostname** — e.g. SRV target `My-MacBook.local`
3. **src_ip** — display and correlation only; not the sole primary key

MAC sources:

- **Scanner (recommended on WSL/LAN):** reads `/proc/net/arp` for `src_ip` and sends optional `mac` on each POST when known
- **API on the host:** same ARP/neighbor lookup as fallback when POST has no `mac`
- **API in Docker (bridge):** often cannot resolve LAN MACs; use host-network scanner + optional `mac` in JSON

Optional JSON field on ingest (backward compatible):

```json
{
  "timestamp_ms": 1700000000000,
  "src_ip": "192.168.1.10",
  "mac": "aa:bb:cc:dd:ee:ff",
  "records": [ ... ]
}
```

## Stored services (service types)

`GET /v1/devices` → `services` lists **Bonjour service types** only (`_airplay._tcp.local`, etc.), matching the scanner console line `services:` under each device summary. Instance names (e.g. `Living Room._airplay._tcp.local`) are not stored separately.

Baselines and `BASELINE_MISMATCH` alerts compare these type sets.

## Security alerts (home-lab semantics)

| Type | When |
|------|------|
| `UNKNOWN_DEVICE` | Non-`APPROVED` device with meaningful mDNS identity (hostname and/or service types). Deduped per device. |
| `BASELINE_MISMATCH` | `APPROVED` device whose hostname and/or service types differ from the approve-time snapshot |

**Honest limits:** Passive mDNS is unauthenticated. Alerts mean **baseline mismatch**, not forensic anti-spoofing. Discovery only includes hosts that **advertise mDNS** while the scanner runs. MAC lookup does not discover silent hosts.

## mDNS host preference

When upserting `devices.mdns_host` from record `host` fields:

1. **SRV** (`type_name == "SRV"`) — overwrites any existing host.
2. **Other types (e.g. A)** — set host only if `mdns_host` is currently empty.

This mirrors the C++ `DeviceRegistry` merge rules (`api/app/mdns_merge.py`, `api/app/mdns_names.py`).

## Database (development)

Single schema file: [`migrations/schema.sql`](migrations/schema.sql). On first API start the DB is created automatically.

**After upgrading** to a build that changes stored service semantics (service types only), reset once so old mixed rows are cleared:

```bash
# stop uvicorn first
python3 scripts/lan_db.py reset
```

If the schema file or `SCHEMA_VERSION` in `app/db.py` changes, reset the same way.

```bash
python3 scripts/lan_db.py dump
python3 scripts/lan_db.py reset
```

Honors `LAN_DB_PATH`.

## Tests

```bash
cd api && source .venv/bin/activate && pytest tests/ -v
```
