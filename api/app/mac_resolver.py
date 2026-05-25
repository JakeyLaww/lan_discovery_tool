import re
import subprocess
from pathlib import Path

_MAC_RE = re.compile(
    r"^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$"
)


def normalize_mac(value: str | None) -> str | None:
    if not value:
        return None
    cleaned = value.strip().upper().replace("-", ":")
    if not _MAC_RE.match(cleaned):
        return None
    return cleaned


def lookup(src_ip: str, *, arp_path: Path | None = None) -> str | None:
    mac = _lookup_proc_arp(src_ip, arp_path=arp_path)
    if mac:
        return mac
    return _lookup_ip_neigh(src_ip)


def _lookup_proc_arp(src_ip: str, *, arp_path: Path | None = None) -> str | None:
    path = arp_path if arp_path is not None else Path("/proc/net/arp")
    if not path.is_file():
        return None
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError:
        return None
    for line in lines[1:]:
        parts = line.split()
        if len(parts) < 4:
            continue
        ip, _hw_type, _flags, mac = parts[0], parts[1], parts[2], parts[3]
        if ip != src_ip:
            continue
        if mac in ("00:00:00:00:00:00", "<incomplete>"):
            continue
        return normalize_mac(mac)
    return None


def _lookup_ip_neigh(src_ip: str) -> str | None:
    try:
        result = subprocess.run(
            ["ip", "-4", "neigh", "show", src_ip],
            capture_output=True,
            text=True,
            timeout=2,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    if result.returncode != 0 or not result.stdout.strip():
        return None
    for line in result.stdout.splitlines():
        parts = line.split()
        if not parts or parts[0] != src_ip:
            continue
        for token in parts:
            if token.startswith("lladdr"):
                idx = parts.index(token)
                if idx + 1 < len(parts):
                    return normalize_mac(parts[idx + 1])
            if _MAC_RE.match(token):
                return normalize_mac(token)
    return None
