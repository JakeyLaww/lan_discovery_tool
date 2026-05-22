# LAN Discovery Tool

[![CI](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/ci.yml/badge.svg)](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/ci.yml)

`mDNS scanner`:

- Periodically browses `_services._dns-sd._udp.local`
- Listens for multicast responses
- Prints discovered PTR/SRV/TXT/A records to the console
- Change detection suppresses duplicate lines for unchanged records

## Build

```bash
./scripts/build.sh          # incremental (fast day-to-day)
./scripts/build.sh --clean  # wipe build/ and full rebuild
```

## Run

```bash
./build/scanner
```

### CLI options

| Flag | Effect |
|------|--------|
| (default) | Log level INFO |
| `-d`, `--debug` | DEBUG diagnostics (broadcasts, ignored queries) |
| `-q`, `--quiet` | WARN and ERROR only |
| `-v`, `--verbose` | Wire hex dumps (use with `-d`) |
| `-I`, `-i`, `--interface <name>` | Pin multicast send/receive to one NIC (e.g. `eth0`) |
| `-h`, `--help` | Usage |

On startup the scanner logs **local IPv4 interfaces** for diagnostics.


## Architecture

- `DiscoveryEngine`: threads, periodic PTR browse, queue
- `NetworkSocket`: multicast bind, optional interface, loopback off
- `MdnsPacketInterpreter`: decode; skip DNS queries (QR=0)
- `DeviceStateStore` + `ChangeFilteredEventSink`: emit on change only
- `StdoutEventSink`: human-readable discovery lines

## Tests

```bash
ctest --test-dir build
```

Unit tests use synthetic DNS/mDNS packets only (no network). CI runs the same tests on every push and PR; it does not perform live LAN discovery—GitHub-hosted runners are not on your Wi‑Fi multicast segment.

## CI

- **CI** (`.github/workflows/ci.yml`): clean build via `./scripts/build.sh --clean`, `ctest`, and `./build/scanner --help` smoke test
- **CodeQL** (`.github/workflows/codeql-analysis.yml`): static security analysis for C++
