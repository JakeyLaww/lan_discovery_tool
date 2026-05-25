import sqlite3
import uuid
from typing import Any

from app.mdns_merge import parse_services_json, services_json
from app.models import AlertOut, DeviceDetailOut, DeviceOut


def find_device_by_mac(conn: sqlite3.Connection, mac: str) -> sqlite3.Row | None:
    return conn.execute(
        "SELECT * FROM devices WHERE mac = ?",
        (mac,),
    ).fetchone()


def find_device_by_src_ip(conn: sqlite3.Connection, src_ip: str) -> sqlite3.Row | None:
    return conn.execute(
        "SELECT * FROM devices WHERE src_ip = ? ORDER BY last_seen_ms DESC LIMIT 1",
        (src_ip,),
    ).fetchone()


def insert_device(
    conn: sqlite3.Connection,
    *,
    device_id: str,
    src_ip: str,
    mdns_host: str | None,
    mac: str | None,
    status: str,
    first_seen_ms: int,
    last_seen_ms: int,
) -> None:
    conn.execute(
        """
        INSERT INTO devices (
            device_id, src_ip, mdns_host, mac, display_name, status,
            first_seen_ms, last_seen_ms
        ) VALUES (?, ?, ?, ?, NULL, ?, ?, ?)
        """,
        (device_id, src_ip, mdns_host, mac, status, first_seen_ms, last_seen_ms),
    )


def update_device_observation(
    conn: sqlite3.Connection,
    device_id: str,
    *,
    src_ip: str,
    mdns_host: str | None,
    last_seen_ms: int,
    mac: str | None = None,
) -> None:
    if mac is not None:
        conn.execute(
            """
            UPDATE devices
            SET src_ip = ?, mdns_host = ?, last_seen_ms = ?, mac = ?
            WHERE device_id = ?
            """,
            (src_ip, mdns_host, last_seen_ms, mac, device_id),
        )
    else:
        conn.execute(
            """
            UPDATE devices
            SET src_ip = ?, mdns_host = ?, last_seen_ms = ?
            WHERE device_id = ?
            """,
            (src_ip, mdns_host, last_seen_ms, device_id),
        )


def update_device_mac(
    conn: sqlite3.Connection,
    device_id: str,
    mac: str,
) -> None:
    conn.execute(
        "UPDATE devices SET mac = ? WHERE device_id = ?",
        (mac, device_id),
    )


def upsert_service_keys(
    conn: sqlite3.Connection,
    device_id: str,
    service_keys: list[str],
) -> None:
    for key in service_keys:
        conn.execute(
            """
            INSERT OR IGNORE INTO services (device_id, service_key)
            VALUES (?, ?)
            """,
            (device_id, key),
        )


def append_discovery_event(
    conn: sqlite3.Connection,
    device_id: str,
    received_at_ms: int,
    payload_json: str,
) -> None:
    conn.execute(
        """
        INSERT INTO discovery_events (device_id, received_at_ms, payload_json)
        VALUES (?, ?, ?)
        """,
        (device_id, received_at_ms, payload_json),
    )


def list_services_for_device(conn: sqlite3.Connection, device_id: str) -> list[str]:
    rows = conn.execute(
        "SELECT service_key FROM services WHERE device_id = ? ORDER BY service_key",
        (device_id,),
    ).fetchall()
    return [r["service_key"] for r in rows]


def list_devices(conn: sqlite3.Connection) -> list[DeviceOut]:
    rows = conn.execute(
        """
        SELECT device_id, src_ip, mdns_host, mac, display_name, status,
               first_seen_ms, last_seen_ms
        FROM devices
        ORDER BY last_seen_ms DESC
        """
    ).fetchall()
    return [_row_to_device(conn, row) for row in rows]


def get_device(conn: sqlite3.Connection, device_id: str) -> DeviceDetailOut | None:
    row = conn.execute(
        """
        SELECT device_id, src_ip, mdns_host, mac, display_name, status,
               first_seen_ms, last_seen_ms
        FROM devices WHERE device_id = ?
        """,
        (device_id,),
    ).fetchone()
    if row is None:
        return None
    return _row_to_device(conn, row)


def get_device_row(conn: sqlite3.Connection, device_id: str) -> sqlite3.Row | None:
    return conn.execute(
        "SELECT * FROM devices WHERE device_id = ?",
        (device_id,),
    ).fetchone()


def set_device_approved(
    conn: sqlite3.Connection,
    device_id: str,
    *,
    display_name: str | None,
) -> bool:
    row = get_device_row(conn, device_id)
    if row is None:
        return False
    conn.execute(
        """
        UPDATE devices
        SET status = 'APPROVED', display_name = COALESCE(?, display_name)
        WHERE device_id = ?
        """,
        (display_name, device_id),
    )
    return True


def upsert_baseline(
    conn: sqlite3.Connection,
    device_id: str,
    *,
    mdns_host: str | None,
    services: list[str],
    captured_at_ms: int,
) -> None:
    conn.execute(
        """
        INSERT INTO device_baselines (device_id, mdns_host, services_json, captured_at_ms)
        VALUES (?, ?, ?, ?)
        ON CONFLICT(device_id) DO UPDATE SET
            mdns_host = excluded.mdns_host,
            services_json = excluded.services_json,
            captured_at_ms = excluded.captured_at_ms
        """,
        (device_id, mdns_host, services_json(services), captured_at_ms),
    )


def get_baseline(conn: sqlite3.Connection, device_id: str) -> sqlite3.Row | None:
    return conn.execute(
        "SELECT * FROM device_baselines WHERE device_id = ?",
        (device_id,),
    ).fetchone()


def insert_alert_if_new(
    conn: sqlite3.Connection,
    *,
    device_id: str,
    alert_type: str,
    message: str,
    dedupe_key: str,
    observed_at_ms: int,
    severity: str = "warn",
) -> bool:
    alert_id = str(uuid.uuid4())
    cursor = conn.execute(
        """
        INSERT OR IGNORE INTO alerts (
            alert_id, device_id, alert_type, message, severity,
            observed_at_ms, dedupe_key
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
        """,
        (
            alert_id,
            device_id,
            alert_type,
            message,
            severity,
            observed_at_ms,
            dedupe_key,
        ),
    )
    return cursor.rowcount > 0


def list_alerts(
    conn: sqlite3.Connection,
    *,
    limit: int = 100,
    alert_type: str | None = None,
) -> list[AlertOut]:
    if alert_type:
        rows = conn.execute(
            """
            SELECT alert_id, device_id, alert_type, message, severity,
                   observed_at_ms, dedupe_key, resolved_at_ms
            FROM alerts
            WHERE alert_type = ?
            ORDER BY observed_at_ms DESC
            LIMIT ?
            """,
            (alert_type, limit),
        ).fetchall()
    else:
        rows = conn.execute(
            """
            SELECT alert_id, device_id, alert_type, message, severity,
                   observed_at_ms, dedupe_key, resolved_at_ms
            FROM alerts
            ORDER BY observed_at_ms DESC
            LIMIT ?
            """,
            (limit,),
        ).fetchall()
    return [
        AlertOut(
            alert_id=r["alert_id"],
            device_id=r["device_id"],
            alert_type=r["alert_type"],
            message=r["message"],
            severity=r["severity"],
            observed_at_ms=r["observed_at_ms"],
            dedupe_key=r["dedupe_key"],
            resolved_at_ms=r["resolved_at_ms"],
        )
        for r in rows
    ]


def _row_to_device(conn: sqlite3.Connection, row: Any) -> DeviceOut:
    services = list_services_for_device(conn, row["device_id"])
    return DeviceOut(
        device_id=row["device_id"],
        src_ip=row["src_ip"],
        mdns_host=row["mdns_host"],
        mac=row["mac"],
        display_name=row["display_name"],
        status=row["status"],
        first_seen_ms=row["first_seen_ms"],
        last_seen_ms=row["last_seen_ms"],
        services=services,
    )
