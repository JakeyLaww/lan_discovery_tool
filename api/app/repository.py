import json
import sqlite3
import time
import uuid
from typing import Any

from app.models import DiscoveryEventIn, DeviceDetailOut, DeviceOut


def _merge_mdns_host(current: str | None, candidate: str | None, *, from_srv: bool) -> str | None:
    if not candidate:
        return current
    if from_srv:
        return candidate
    if current:
        return current
    return candidate


def _event_seen_ms(event: DiscoveryEventIn) -> int:
    if event.records:
        return max(event.timestamp_ms, max(r.last_seen_ms for r in event.records))
    return event.timestamp_ms


def upsert_discovery_event(conn: sqlite3.Connection, event: DiscoveryEventIn) -> str:
    if not event.src_ip:
        raise ValueError("src_ip is required")

    seen_ms = _event_seen_ms(event)
    received_ms = int(time.time() * 1000)
    payload_json = json.dumps(event.model_dump())

    row = conn.execute(
        "SELECT device_id, mdns_host FROM devices WHERE src_ip = ?",
        (event.src_ip,),
    ).fetchone()

    if row is None:
        device_id = str(uuid.uuid4())
        mdns_host: str | None = None
        for rec in event.records:
            mdns_host = _merge_mdns_host(
                mdns_host,
                rec.host,
                from_srv=(rec.type_name == "SRV"),
            )
        conn.execute(
            """
            INSERT INTO devices (
                device_id, src_ip, mdns_host, mac, status,
                first_seen_ms, last_seen_ms
            ) VALUES (?, ?, ?, NULL, 'UNKNOWN', ?, ?)
            """,
            (device_id, event.src_ip, mdns_host, seen_ms, seen_ms),
        )
    else:
        device_id = row["device_id"]
        mdns_host = row["mdns_host"]
        for rec in event.records:
            mdns_host = _merge_mdns_host(
                mdns_host,
                rec.host,
                from_srv=(rec.type_name == "SRV"),
            )
        conn.execute(
            """
            UPDATE devices
            SET last_seen_ms = ?, mdns_host = ?
            WHERE device_id = ?
            """,
            (seen_ms, mdns_host, device_id),
        )

    for rec in event.records:
        if rec.service:
            conn.execute(
                """
                INSERT OR IGNORE INTO services (device_id, service_key)
                VALUES (?, ?)
                """,
                (device_id, rec.service),
            )

    conn.execute(
        """
        INSERT INTO discovery_events (device_id, received_at_ms, payload_json)
        VALUES (?, ?, ?)
        """,
        (device_id, received_ms, payload_json),
    )
    return device_id


def list_devices(conn: sqlite3.Connection) -> list[DeviceOut]:
    rows = conn.execute(
        """
        SELECT device_id, src_ip, mdns_host, mac, status, first_seen_ms, last_seen_ms
        FROM devices
        ORDER BY last_seen_ms DESC
        """
    ).fetchall()
    return [_row_to_device(conn, row) for row in rows]


def get_device(conn: sqlite3.Connection, device_id: str) -> DeviceDetailOut | None:
    row = conn.execute(
        """
        SELECT device_id, src_ip, mdns_host, mac, status, first_seen_ms, last_seen_ms
        FROM devices WHERE device_id = ?
        """,
        (device_id,),
    ).fetchone()
    if row is None:
        return None
    return _row_to_device(conn, row)


def _row_to_device(conn: sqlite3.Connection, row: Any) -> DeviceOut:
    services = [
        r["service_key"]
        for r in conn.execute(
            "SELECT service_key FROM services WHERE device_id = ? ORDER BY service_key",
            (row["device_id"],),
        ).fetchall()
    ]
    return DeviceOut(
        device_id=row["device_id"],
        src_ip=row["src_ip"],
        mdns_host=row["mdns_host"],
        mac=row["mac"],
        status=row["status"],
        first_seen_ms=row["first_seen_ms"],
        last_seen_ms=row["last_seen_ms"],
        services=services,
    )
