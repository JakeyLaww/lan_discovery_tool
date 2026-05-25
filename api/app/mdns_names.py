"""Bonjour name classification — mirrors C++ MdnsNames / DeviceRegistry."""

from app.models import DiscoveryRecordIn

META_SERVICE = "_services._dns-sd._udp.local"
_TCP_SUFFIX = "._tcp.local"
_UDP_SUFFIX = "._udp.local"


def label_count(name: str) -> int:
    if not name:
        return 0
    return name.count(".") + 1


def has_mdns_service_suffix(name: str) -> bool:
    return name.endswith(_TCP_SUFFIX) or name.endswith(_UDP_SUFFIX)


def is_service_type_name(name: str) -> bool:
    if not has_mdns_service_suffix(name) or name == META_SERVICE:
        return False
    return label_count(name) == 3


def is_instance_name(name: str) -> bool:
    if not has_mdns_service_suffix(name) or name == META_SERVICE:
        return False
    return label_count(name) >= 4


def service_type_from_name(name: str) -> str | None:
    if is_service_type_name(name):
        return name
    if not is_instance_name(name):
        return None
    transport_pos = name.rfind(_TCP_SUFFIX)
    if transport_pos == -1:
        transport_pos = name.rfind(_UDP_SUFFIX)
    if transport_pos <= 0:
        return None
    dot_before = name.rfind(".", 0, transport_pos)
    if dot_before == -1:
        return None
    return name[dot_before + 1 :]


def _ptr_rdata(rec: DiscoveryRecordIn) -> str:
    return (rec.service or rec.rdata_text or "").strip()


def service_types_from_record(rec: DiscoveryRecordIn) -> set[str]:
    types: set[str] = set()
    type_name = (rec.type_name or "").upper()
    owner = rec.owner_name

    if type_name == "PTR" and owner == META_SERVICE:
        rdata = _ptr_rdata(rec)
        if is_service_type_name(rdata):
            types.add(rdata)
    elif type_name == "PTR" and is_service_type_name(owner):
        rdata = _ptr_rdata(rec)
        if is_instance_name(rdata):
            st = service_type_from_name(rdata)
            if st:
                types.add(st)
    elif type_name in ("SRV", "TXT"):
        st = service_type_from_name(owner)
        if st:
            types.add(st)

    return types


def service_types_from_records(records: list[DiscoveryRecordIn]) -> list[str]:
    keys: set[str] = set()
    for rec in records:
        keys.update(service_types_from_record(rec))
    return sorted(keys)
