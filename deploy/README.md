# Docker deployment

Docker Compose stack: **Discovery API** + **mDNS Scanner**.

## Prerequisites

- Docker Compose v2
- Linux host recommended (scanner needs mDNS multicast)

## Run

From the repository root:

```bash
./scripts/compose_up.sh
```

Or manually:

```bash
cd deploy && docker compose up --build
```

- Discovery API: http://127.0.0.1:8000 (`GET /health`, `GET /v1/devices`)
- SQLite: Docker volume `lan_db` → `/data/lan.db` in the API container

**Fresh stack (wipe SQLite volume and rebuild):**

```bash
./scripts/compose_up.sh --reset-db
```

To inspect the database locally, copy `lan.db` from the volume or use [`scripts/lan_db.py`](../scripts/lan_db.py) against an exported file. See the [root README](../README.md).

## Scanner networking

The scanner service uses `network_mode: host` so mDNS multicast (`224.0.0.251`) reaches your LAN interface.

On WSL2, enable mirrored networking in `.wslconfig` before expecting reliable mDNS from a container.

When the API is published on the host port (default mapping), the scanner should use `LAN_API_URL=http://127.0.0.1:8000`. The compose file sets this for the scanner container via host networking.

## Optional auth

Uncomment and set the same `LAN_API_TOKEN` on both `api` and `scanner` services in [`docker-compose.yml`](docker-compose.yml).
