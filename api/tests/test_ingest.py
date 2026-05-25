import sqlite3
from pathlib import Path

import pytest

from app.db import connect, ensure_database
from app.ingest import approve_device, process_discovery_event
from app.models import DiscoveryEventIn, DiscoveryRecordIn
from app.repository import list_alerts, list_devices
from app.security_policy import ALERT_BASELINE_MISMATCH, ALERT_UNKNOWN_DEVICE


@pytest.fixture
def conn(tmp_path: Path) -> sqlite3.Connection:
    db_path = tmp_path / "ingest.db"
    connection = connect(db_path)
    ensure_database(connection)
    yield connection
    connection.close()


def _srv_event(
    src_ip: str,
    host: str,
    service: str,
    ts: int = 1000,
) -> DiscoveryEventIn:
    return DiscoveryEventIn(
        timestamp_ms=ts,
        src_ip=src_ip,
        records=[
            DiscoveryRecordIn(
                owner_name=service,
                type=33,
                type_name="SRV",
                rdata_text=f"target={host}",
                ttl=120,
                last_seen_ms=ts,
                service=service,
                host=host,
            )
        ],
    )


def test_unknown_alert_on_first_meaningful_discovery(conn: sqlite3.Connection) -> None:
    process_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "svc._tcp.local")
    )
    conn.commit()
    alerts = list_alerts(conn)
    assert len(alerts) == 1
    assert alerts[0].alert_type == ALERT_UNKNOWN_DEVICE


def test_no_duplicate_unknown_alert_on_repost(conn: sqlite3.Connection) -> None:
    ev = _srv_event("10.0.0.5", "tv.local", "svc._tcp.local")
    process_discovery_event(conn, ev)
    process_discovery_event(conn, ev)
    conn.commit()
    alerts = list_alerts(conn, alert_type=ALERT_UNKNOWN_DEVICE)
    assert len(alerts) == 1


def test_approve_captures_baseline(conn: sqlite3.Connection) -> None:
    process_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "svc._tcp.local")
    )
    conn.commit()
    device_id = list_devices(conn)[0].device_id
    approve_device(conn, device_id, display_name="Living Room TV")
    conn.commit()
    row = conn.execute(
        "SELECT mdns_host, services_json FROM device_baselines WHERE device_id = ?",
        (device_id,),
    ).fetchone()
    assert row is not None
    assert row["mdns_host"] == "tv.local"
    assert "svc._tcp.local" in row["services_json"]
    device = list_devices(conn)[0]
    assert device.status == "APPROVED"
    assert device.display_name == "Living Room TV"


def test_baseline_mismatch_after_hostname_change(conn: sqlite3.Connection) -> None:
    process_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "svc._tcp.local")
    )
    conn.commit()
    device_id = list_devices(conn)[0].device_id
    approve_device(conn, device_id, display_name=None)
    conn.commit()

    process_discovery_event(
        conn, _srv_event("10.0.0.5", "renamed.local", "svc._tcp.local", ts=2000)
    )
    conn.commit()

    mismatch = list_alerts(conn, alert_type=ALERT_BASELINE_MISMATCH)
    assert len(mismatch) == 1
    assert "renamed" in mismatch[0].message or "hostname" in mismatch[0].message
