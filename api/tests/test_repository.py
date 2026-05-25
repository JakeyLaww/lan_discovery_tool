import sqlite3
from pathlib import Path

import pytest

from app.db import connect, ensure_database
from app.ingest import process_discovery_event
from app.models import DiscoveryEventIn, DiscoveryRecordIn
from app.repository import get_device, list_devices


@pytest.fixture
def conn(tmp_path: Path) -> sqlite3.Connection:
    db_path = tmp_path / "test.db"
    connection = connect(db_path)
    ensure_database(connection)
    yield connection
    connection.close()


def _srv_event(
    src_ip: str,
    host: str,
    service: str,
    ts: int = 1000,
    mac: str | None = None,
) -> DiscoveryEventIn:
    return DiscoveryEventIn(
        timestamp_ms=ts,
        src_ip=src_ip,
        mac=mac,
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


def test_ingest_creates_device_with_uuid(conn: sqlite3.Connection) -> None:
    device_id = process_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "Living Room._airplay._tcp.local")
    )
    conn.commit()
    assert len(device_id) == 36
    devices = list_devices(conn)
    assert len(devices) == 1
    assert devices[0].src_ip == "10.0.0.5"
    assert devices[0].mdns_host == "tv.local"
    assert devices[0].status == "UNKNOWN"


def test_ingest_same_src_ip_updates_last_seen(conn: sqlite3.Connection) -> None:
    process_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "svc._tcp.local", ts=1000)
    )
    process_discovery_event(
        conn, _srv_event("10.0.0.5", "tv.local", "svc._tcp.local", ts=5000)
    )
    conn.commit()
    devices = list_devices(conn)
    assert len(devices) == 1
    assert devices[0].last_seen_ms == 5000


def test_srv_host_overwrites_a_host(conn: sqlite3.Connection) -> None:
    process_discovery_event(
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
    process_discovery_event(
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
    process_discovery_event(conn, ev)
    process_discovery_event(conn, ev)
    conn.commit()
    device = get_device(conn, list_devices(conn)[0].device_id)
    assert device is not None
    assert device.services == ["svc._tcp.local"]


def test_ingest_sets_mac_from_event(conn: sqlite3.Connection) -> None:
    mac = "AA:BB:CC:DD:EE:FF"
    process_discovery_event(
        conn,
        _srv_event("10.0.0.5", "tv.local", "svc._tcp.local", mac=mac),
    )
    conn.commit()
    device = list_devices(conn)[0]
    assert device.mac == mac


def test_mac_first_same_device_new_src_ip(conn: sqlite3.Connection) -> None:
    mac = "AA:BB:CC:DD:EE:FF"
    process_discovery_event(
        conn,
        _srv_event("10.0.0.5", "tv.local", "svc._tcp.local", mac=mac),
    )
    first_id = list_devices(conn)[0].device_id
    process_discovery_event(
        conn,
        _srv_event("10.0.0.9", "tv.local", "svc._tcp.local", mac=mac),
    )
    conn.commit()
    devices = list_devices(conn)
    assert len(devices) == 1
    assert devices[0].device_id == first_id
    assert devices[0].src_ip == "10.0.0.9"
    assert devices[0].mac == mac
