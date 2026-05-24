import sqlite3
from pathlib import Path

import pytest

from app.db import apply_migrations, connect
from app.models import DiscoveryEventIn, DiscoveryRecordIn
from app.repository import get_device, list_devices, upsert_discovery_event


@pytest.fixture
def conn(tmp_path: Path) -> sqlite3.Connection:
    db_path = tmp_path / "test.db"
    connection = connect(db_path)
    apply_migrations(connection)
    yield connection
    connection.close()


def _srv_event(src_ip: str, host: str, service: str, ts: int = 1000) -> DiscoveryEventIn:
    return DiscoveryEventIn(
        timestamp_ms=ts,
        src_ip=src_ip,
        records=[
            DiscoveryRecordIn(
                owner_name=service,
                type=33,
                type_name="SRV",
                rdata_text=f"priority=0 weight=0 port=7000 target={host}",
                ttl=120,
                last_seen_ms=ts,
                service=service,
                host=host,
            )
        ],
    )


def test_upsert_creates_device_with_uuid(conn: sqlite3.Connection) -> None:
    device_id = upsert_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "Living Room._airplay._tcp.local")
    )
    conn.commit()
    assert len(device_id) == 36
    devices = list_devices(conn)
    assert len(devices) == 1
    assert devices[0].src_ip == "10.0.0.5"
    assert devices[0].mdns_host == "tv.local"
    assert devices[0].status == "UNKNOWN"


def test_upsert_same_src_ip_updates_last_seen(conn: sqlite3.Connection) -> None:
    upsert_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "svc._tcp.local", ts=1000)
    )
    upsert_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "svc._tcp.local", ts=5000)
    )
    conn.commit()
    devices = list_devices(conn)
    assert len(devices) == 1
    assert devices[0].last_seen_ms == 5000


def test_srv_host_overwrites_a_host(conn: sqlite3.Connection) -> None:
    upsert_discovery_event(
        conn,
        DiscoveryEventIn(
            timestamp_ms=1000,
            src_ip="10.0.0.5",
            records=[
                DiscoveryRecordIn(
                    owner_name="tv.local",
                    type=1,
                    type_name="A",
                    rdata_text="10.0.0.5",
                    ttl=120,
                    last_seen_ms=1000,
                    host="tv.local",
                )
            ],
        ),
    )
    upsert_discovery_event(
        conn,
        DiscoveryEventIn(
            timestamp_ms=2000,
            src_ip="10.0.0.5",
            records=[
                DiscoveryRecordIn(
                    owner_name="Living._airplay._tcp.local",
                    type=33,
                    type_name="SRV",
                    rdata_text="target=macbook.local",
                    ttl=120,
                    last_seen_ms=2000,
                    host="macbook.local",
                )
            ],
        ),
    )
    conn.commit()
    device = get_device(conn, list_devices(conn)[0].device_id)
    assert device is not None
    assert device.mdns_host == "macbook.local"


def test_services_deduped(conn: sqlite3.Connection) -> None:
    ev = _srv_event("10.0.0.5", "tv.local", "svc._tcp.local")
    upsert_discovery_event(conn, ev)
    upsert_discovery_event(conn, ev)
    conn.commit()
    device = get_device(conn, list_devices(conn)[0].device_id)
    assert device is not None
    assert device.services == ["svc._tcp.local"]
