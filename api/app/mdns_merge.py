import hashlib
import json

from app.models import DiscoveryEventIn, DiscoveryRecordIn


def merge_mdns_host(
    current: str | None, candidate: str | None, *, from_srv: bool
) -> str | None:
    if not candidate:
        return current
    if from_srv:
        return candidate
    if current:
        return current
    return candidate


def mdns_host_from_event(event: DiscoveryEventIn) -> str | None:
    host: str | None = None
    for rec in event.records:
        host = merge_mdns_host(host, rec.host, from_srv=(rec.type_name == "SRV"))
    return host


def service_keys_from_records(records: list[DiscoveryRecordIn]) -> list[str]:
    """Bonjour service types only (aligned with C++ DeviceRegistry service_types)."""
    from app.mdns_names import service_types_from_records

    return service_types_from_records(records)


def services_json(service_keys: list[str]) -> str:
    return json.dumps(service_keys)


def parse_services_json(raw: str | None) -> list[str]:
    if not raw:
        return []
    data = json.loads(raw)
    if not isinstance(data, list):
        return []
    return sorted(str(x) for x in data)


def service_sets_equal(a: list[str], b: list[str]) -> bool:
    return sorted(a) == sorted(b)


def diff_fingerprint(mdns_host: str | None, services: list[str]) -> str:
    payload = json.dumps(
        {"mdns_host": mdns_host or "", "services": sorted(services)},
        sort_keys=True,
    )
    return hashlib.sha256(payload.encode()).hexdigest()[:16]
