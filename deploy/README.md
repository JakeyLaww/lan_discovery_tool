# Docker deployment

Phase 2 compose stack: API + headless scanner.

## Prerequisites

- Docker Compose v2
- Linux host recommended for scanner mDNS (multicast)

## Run

```bash
# from repo root (recommended)
./scripts/compose_up.sh

# or manually
cd deploy && docker compose up --build
```

- API: http://127.0.0.1:8000 (`GET /health`, `GET /v1/devices`)
- SQLite volume: `lan_db` → `/data/lan.db` in the API container

**Fresh stack (wipe SQLite volume and rebuild):**

```bash
./scripts/compose_up.sh --reset-db
```

Inspect the DB inside the API container: `docker compose -f deploy/docker-compose.yml exec api python3 -c "..."` or copy `/data/lan.db` out; for local dev use `python3 scripts/lan_db.py dump` against `api/data/lan.db`.

## Scanner networking

The scanner service uses `network_mode: host` so mDNS multicast (224.0.0.251) reaches your LAN interface.

On WSL2, use mirrored networking in `.wslconfig` before expecting reliable mDNS from a container.

Set `LAN_API_URL` to `http://127.0.0.1:8000` when the API is published on the host port (default compose mapping).

## Optional auth

Uncomment and set the same `LAN_API_TOKEN` on both `api` and `scanner` services in `docker-compose.yml`.
