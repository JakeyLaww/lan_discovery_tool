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

On WSL2, use mirrored networking (`networkingMode=mirrored` in `.wslconfig`, then `wsl --shutdown`) so the scanner shares your Wi‑Fi subnet. mDNS only sees hosts that advertise while the scanner runs.

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
| `--max-probes-per-tick <n>` | Follow-up mDNS probes per poll tick (default 8) |
| `--probe-cooldown-ms <ms>` | Minimum ms before re-probing the same name (default 60000) |
| `-h`, `--help` | Usage |

On startup the scanner logs **local IPv4 interfaces** for diagnostics.


## Architecture

- `DiscoveryEngine`: threads, meta browse, `MdnsProbePlanner` follow-up queue, packet queue
- `MdnsProbePlanner`: schedules service/instance probes (worker enqueues, poller sends)
- `NetworkSocket`: multicast bind, optional interface, loopback off
- `MdnsPacketInterpreter`: decode; skip DNS queries (QR=0)
- `DeviceStateStore` + `ChangeFilteredEventSink`: emit on change only
- `DeviceRegistry` + `DeviceSummaryEventSink`: aggregate by `src_ip`, log device block on change
- `StdoutEventSink`: human-readable discovery lines

## Tests

```bash
ctest --test-dir build
```