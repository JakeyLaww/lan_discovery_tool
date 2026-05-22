# LAN Discovery Tool

[![Build and Test](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/build-and-test.yml)
[![CodeQL Analysis](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/JakeyLaww/lan_discovery_tool/actions/workflows/codeql-analysis.yml)

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

Unit tests use synthetic DNS/mDNS packets only (no network). CI does not perform live LAN discovery.

## Continuous integration

| Workflow | File | What it runs |
|----------|------|----------------|
| **Build and Test** | [`.github/workflows/build-and-test.yml`](.github/workflows/build-and-test.yml) | Clean build, `ctest`, `./build/scanner --help` |
| **CodeQL Analysis** | [`.github/workflows/codeql-analysis.yml`](.github/workflows/codeql-analysis.yml) | Static security analysis for C++ |
