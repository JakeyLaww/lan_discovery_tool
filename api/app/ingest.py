import json
import sqlite3
import time
import uuid

from app import mac_resolver, repository
from app.mdns_merge import (
    mdns_host_from_event,
    merge_mdns_host,
    parse_services_json,
    service_keys_from_records,
)
from app.models import DiscoveryEventIn
from app.security_policy import (
    BaselineState,
    DevicePolicyState,
    evaluate_security_policy,
)


def _event_seen_ms(event: DiscoveryEventIn) -> int:
    if event.records:
        return max(event.timestamp_ms, max(r.last_seen_ms for r in event.records))
    return event.timestamp_ms


def _resolve_mac_hint(event: DiscoveryEventIn) -> str | None:
    if event.mac:
        return mac_resolver.normalize_mac(event.mac)
    return mac_resolver.lookup(event.src_ip)


def resolve_device_id(
    conn: sqlite3.Connection,
    event: DiscoveryEventIn,
    mac_hint: str | None,
) -> str:
    seen_ms = _event_seen_ms(event)
    mdns_host = mdns_host_from_event(event)

    if mac_hint:
        row = repository.find_device_by_mac(conn, mac_hint)
        if row is not None:
            device_id = row["device_id"]
            merged_host = mdns_host
            if row["mdns_host"] and mdns_host:
                merged_host = merge_mdns_host(
                    row["mdns_host"], mdns_host, from_srv=True
                ) or mdns_host
            elif row["mdns_host"] and not mdns_host:
                merged_host = row["mdns_host"]
            repository.update_device_observation(
                conn,
                device_id,
                src_ip=event.src_ip,
                mdns_host=merged_host,
                last_seen_ms=seen_ms,
                mac=mac_hint,
            )
            return device_id

    row = repository.find_device_by_src_ip(conn, event.src_ip)
    if row is not None:
        device_id = row["device_id"]
        current_host = row["mdns_host"]
        for rec in event.records:
            current_host = merge_mdns_host(
                current_host,
                rec.host,
                from_srv=(rec.type_name == "SRV"),
            )
        repository.update_device_observation(
            conn,
            device_id,
            src_ip=event.src_ip,
            mdns_host=current_host,
            last_seen_ms=seen_ms,
            mac=mac_hint if mac_hint and not row["mac"] else None,
        )
        if mac_hint and not row["mac"]:
            repository.update_device_mac(conn, device_id, mac_hint)
        return device_id

    device_id = str(uuid.uuid4())
    repository.insert_device(
        conn,
        device_id=device_id,
        src_ip=event.src_ip,
        mdns_host=mdns_host,
        mac=mac_hint,
        status="UNKNOWN",
        first_seen_ms=seen_ms,
        last_seen_ms=seen_ms,
    )
    return device_id


def _attach_mac_if_needed(
    conn: sqlite3.Connection,
    device_id: str,
    src_ip: str,
    event_mac: str | None,
) -> None:
    row = repository.get_device_row(conn, device_id)
    if row is None or row["mac"]:
        return
    mac = mac_resolver.normalize_mac(event_mac) if event_mac else mac_resolver.lookup(src_ip)
    if mac:
        repository.update_device_mac(conn, device_id, mac)


def _apply_policy_alerts(
    conn: sqlite3.Connection,
    device_id: str,
    observed_at_ms: int,
) -> None:
    row = repository.get_device_row(conn, device_id)
    if row is None:
        return

    services = repository.list_services_for_device(conn, device_id)
    device_state = DevicePolicyState(
        device_id=device_id,
        status=row["status"],
        mdns_host=row["mdns_host"],
        services=services,
    )

    baseline_row = repository.get_baseline(conn, device_id)
    baseline = None
    if baseline_row is not None:
        baseline = BaselineState(
            mdns_host=baseline_row["mdns_host"],
            services=parse_services_json(baseline_row["services_json"]),
        )

    for alert in evaluate_security_policy(device_state, baseline):
        repository.insert_alert_if_new(
            conn,
            device_id=device_id,
            alert_type=alert.alert_type,
            message=alert.message,
            dedupe_key=alert.dedupe_key,
            observed_at_ms=observed_at_ms,
            severity=alert.severity,
        )


def process_discovery_event(
    conn: sqlite3.Connection,
    event: DiscoveryEventIn,
) -> str:
    if not event.src_ip:
        raise ValueError("src_ip is required")

    seen_ms = _event_seen_ms(event)
    received_ms = int(time.time() * 1000)
    payload_json = json.dumps(event.model_dump())

    mac_hint = _resolve_mac_hint(event)
    device_id = resolve_device_id(conn, event, mac_hint)

    service_keys = service_keys_from_records(event.records)
    repository.upsert_service_keys(conn, device_id, service_keys)
    _attach_mac_if_needed(conn, device_id, event.src_ip, event.mac)

    repository.append_discovery_event(conn, device_id, received_ms, payload_json)
    _apply_policy_alerts(conn, device_id, received_ms)
    return device_id


def approve_device(
    conn: sqlite3.Connection,
    device_id: str,
    *,
    display_name: str | None,
) -> bool:
    if not repository.set_device_approved(conn, device_id, display_name=display_name):
        return False

    row = repository.get_device_row(conn, device_id)
    if row is None:
        return False

    services = repository.list_services_for_device(conn, device_id)
    captured_at_ms = int(time.time() * 1000)
    repository.upsert_baseline(
        conn,
        device_id,
        mdns_host=row["mdns_host"],
        services=services,
        captured_at_ms=captured_at_ms,
    )
    return True
