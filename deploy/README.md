# Docker deployment

Phase 2 compose stack: API + headless scanner.

## Prerequisites

- Docker Compose v2
- Linux host recommended for scanner mDNS (multicast)

## Run

```bash
cd deploy
docker compose up --build
```

- API: http://127.0.0.1:8000 (`GET /health`, `GET /v1/devices`)
- SQLite volume: `lan_db` → `/data/lan.db` in the API container

## Scanner networking

The scanner service uses `network_mode: host` so mDNS multicast (224.0.0.251) reaches your LAN interface.

On WSL2, use mirrored networking in `.wslconfig` before expecting reliable mDNS from a container.

Set `LAN_API_URL` to `http://127.0.0.1:8000` when the API is published on the host port (default compose mapping).

## Optional auth

Uncomment and set the same `LAN_API_TOKEN` on both `api` and `scanner` services in `docker-compose.yml`.
