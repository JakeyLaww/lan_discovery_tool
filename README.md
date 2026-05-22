# LAN Discovery Tool

Phase 1 mDNS scanner: periodically browses `_services._dns-sd._udp.local`, listens for multicast responses, and prints discovered PTR/SRV/TXT/A records to the console. Change detection suppresses duplicate lines for unchanged records.

## Build

```bash
./scripts/build.sh
```

Or manually:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Run

```bash
sudo ./build/scanner
```

Multicast bind on port 5353 typically requires elevated privileges on Linux.

### CLI options (runtime)

| Flag | Effect |
|------|--------|
| (default) | Log level INFO — discovery lines only |
| `-d`, `--debug` | Log level DEBUG — ignored queries, broadcasts, undisplayable responses |
| `-q`, `--quiet` | Log level WARN — errors and warnings only |
| `-v`, `--verbose` | Wire-level hex dumps and parse summaries (requires `-d` to see DEBUG output) |
| `-h`, `--help` | Show usage |

Examples:

```bash
sudo ./build/scanner
sudo ./build/scanner -d
sudo ./build/scanner -d -v
```

Press Ctrl+C to stop cleanly.

## Output

Each new or changed record is logged at INFO, for example:

```
[INFO] discovery ts=2026-05-22T14:03:01.234Z src=192.168.1.42 type=PTR owner=_services._dns-sd._udp.local rdata=_airplay._tcp.local service=_airplay._tcp.local
```

## Troubleshooting

### No `discovery` lines, only DEBUG with `-d -v`

If you only see packets with **1 Q, 0 A**, those are **DNS queries** (often your own browse echoed back). The scanner ignores queries and only emits discoveries from **responses** with answer records.

Check:

```bash
ss -ulnp | grep 5353    # avahi or systemd-resolved using port 5353?
ip addr                 # source IP on received packets (e.g. WSL 10.x)
```

On **WSL2**, multicast from other LAN devices often does not reach the scanner socket. Try native Linux, or validate with `avahi-browse -a` on the host.

### Port conflict

If `avahi-daemon` holds 5353, you may bind with `SO_REUSEPORT` but still receive few foreign responses. Consider stopping avahi temporarily for testing.

## Architecture

- `DiscoveryEngine` — receiver, worker, and poller threads
- `MdnsPacketInterpreter` — decode packets; skip non-responses (QR=0)
- `DeviceStateStore` — emit only when record state changes
- `StdoutEventSink` — console output (swap for HTTP sink in Phase 2)
- `to_json(DiscoveryEvent)` — stub serializer for Phase 2 POST payloads

## Tests

```bash
./build/test_parser
./build/test_interpreter
ctest --test-dir build
```
