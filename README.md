# LAN Discovery Tool

[![Build and Test](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/build-and-test.yml)
[![CodeQL Analysis](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/codeql-analysis.yml)

`mDNS scanner`:

- Periodically browses `_services._dns-sd._udp.local` (meta browse)
- Follow-up probes per RFC 6763: service-type PTR, then instance SRV/TXT
- Listens for multicast responses
- Prints Bonjour-relevant PTR/SRV/TXT/A records to the console (services, instances, `.local` hosts—not reverse DNS or Kerberos TXT)
- Change detection suppresses duplicate lines for unchanged records
- Per-record `ttl` and `last_seen`; `device` summary when a host’s mDNS profile changes


## Build

```bash
./scripts/build.sh          # incremental (fast day-to-day)
./scripts/build.sh --clean  # wipe build/ and full rebuild
./scripts/build.sh --reset-db   # delete api/data/lan.db (opt-in; fresh DB on next API start)
```

## Run

```bash
./build/scanner
```

### Phase 2: scanner + API

Terminal 1 — API (see [api/README.md](api/README.md)):

```bash
cd api && python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --host 127.0.0.1 --port 8000
```

Terminal 2 — scanner with persistence:

```bash
./build/scanner --api-url http://127.0.0.1:8000
```

Stdout discovery lines still print. Only **new/changed** records are POSTed.

| Env / flag | Purpose |
|------------|---------|
| `LAN_API_URL` / `--api-url <base>` | API base URL (CLI wins); posts to `/v1/discovery/events` |
| `LAN_API_TOKEN` / `--api-token` | Optional `X-API-Key` when API has `LAN_API_TOKEN` set |

Docker: [deploy/README.md](deploy/README.md).

**Inspect or reset SQLite** (local dev DB, includes audit log not exposed by the API):

```bash
python3 scripts/lan_db.py dump
python3 scripts/lan_db.py reset    # or ./scripts/build.sh --reset-db before building
```

### CLI options for `scanner`

| Flag | Effect |
|------|--------|
| (default) | Log level INFO |
| `-d`, `--debug` | DEBUG diagnostics (broadcasts, ignored queries) |
| `-q`, `--quiet` | WARN and ERROR only |
| `-v`, `--verbose` | Wire hex dumps (use with `-d`) |
| `-I`, `-i`, `--interface <name>` | Pin multicast send/receive to one NIC (e.g. `eth0`) |
| `--max-probes-per-tick <n>` | Follow-up mDNS probes per poll tick (default 8) |
| `--probe-cooldown-ms <ms>` | Minimum ms before re-probing the same name (default 60000) |
| `--api-url <base>` | POST discovery changes to API (`LAN_API_URL`) |
| `--api-token <key>` | Optional `X-API-Key` (`LAN_API_TOKEN`) |
| `-h`, `--help` | Usage |

On startup the scanner logs **local IPv4 interfaces** for diagnostics.


## Architecture

- `DiscoveryEngine`: threads, meta browse, `MdnsProbePlanner` follow-up queue; delegates receive buffering to `PacketReceiveQueue`
- `PacketReceiveQueue`: bounded thread-safe packet deque with drop accounting
- `MdnsProbePlanner`: schedules service/instance probes (worker enqueues, poller sends)
- `NetworkSocket`: multicast bind, optional interface, loopback off
- `MdnsPacketInterpreter`: decode; skip DNS queries (QR=0); `RecordFilter` selects displayable records
- `DeviceStateStore` + `ChangeFilteredEventSink`: suppress duplicate identical records (`filter_unseen`)
- `DeviceRegistry` + `DeviceSummaryEventSink`: aggregate by `src_ip`, log device block on change
- `StdoutEventSink`: human-readable discovery lines
- `HttpEventSink` + `CompositeEventSink`: POST JSON on change when `--api-url` is set (bounded queue, retry)
- **API** (`api/`): FastAPI + SQLite — devices, services, discovery event audit log

## Tests

```bash
ctest --test-dir build
cd api && source .venv/bin/activate && pytest tests/ -v
```