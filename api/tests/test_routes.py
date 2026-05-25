import sqlite3
from pathlib import Path

import pytest
from fastapi.testclient import TestClient

from app.config import settings
from app.db import connect, ensure_database
from app.main import app
from app.security_policy import ALERT_BASELINE_MISMATCH, ALERT_UNKNOWN_DEVICE


@pytest.fixture
def client(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> TestClient:
    db_path = tmp_path / "routes.db"
    monkeypatch.setattr(settings, "db_path", db_path)
    monkeypatch.setattr(settings, "api_token", None)
    conn = connect(db_path)
    ensure_database(conn)
    conn.close()
    with TestClient(app) as test_client:
        yield test_client


def _sample_payload() -> dict:
    return {
        "timestamp_ms": 1700000000000,
        "src_ip": "192.168.1.10",
        "records": [
            {
                "owner_name": "Living Room._airplay._tcp.local",
                "type": 33,
                "type_name": "SRV",
                "rdata_text": "priority=0 weight=0 port=7000 target=tv.local",
                "ttl": 120,
                "last_seen_ms": 1700000000123,
                "service": "Living Room._airplay._tcp.local",
                "host": "tv.local",
            }
        ],
    }


def test_health(client: TestClient) -> None:
    resp = client.get("/health")
    assert resp.status_code == 200
    assert resp.json() == {"status": "ok"}


def test_post_discovery_accepted(client: TestClient) -> None:
    resp = client.post("/v1/discovery/events", json=_sample_payload())
    assert resp.status_code == 202
    assert resp.json() == {"status": "accepted"}


def test_list_devices_after_ingest(client: TestClient) -> None:
    client.post("/v1/discovery/events", json=_sample_payload())
    resp = client.get("/v1/devices")
    assert resp.status_code == 200
    devices = resp.json()
    assert len(devices) == 1
    assert devices[0]["src_ip"] == "192.168.1.10"
    assert devices[0]["mdns_host"] == "tv.local"
    assert "_airplay._tcp.local" in devices[0]["services"]


def test_auth_required_when_token_set(
    client: TestClient, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(settings, "api_token", "secret")
    resp = client.post("/v1/discovery/events", json=_sample_payload())
    assert resp.status_code == 401
    resp = client.post(
        "/v1/discovery/events",
        json=_sample_payload(),
        headers={"X-API-Key": "secret"},
    )
    assert resp.status_code == 202


def test_empty_records_rejected(client: TestClient) -> None:
    resp = client.post(
        "/v1/discovery/events",
        json={"timestamp_ms": 1, "src_ip": "10.0.0.1", "records": []},
    )
    assert resp.status_code == 400


def test_patch_approve_device(client: TestClient) -> None:
    client.post("/v1/discovery/events", json=_sample_payload())
    devices = client.get("/v1/devices").json()
    device_id = devices[0]["device_id"]
    resp = client.patch(
        f"/v1/devices/{device_id}",
        json={"display_name": "TV"},
    )
    assert resp.status_code == 200
    body = resp.json()
    assert body["status"] == "APPROVED"
    assert body["display_name"] == "TV"


def test_get_alerts_after_ingest(client: TestClient) -> None:
    client.post("/v1/discovery/events", json=_sample_payload())
    resp = client.get("/v1/alerts")
    assert resp.status_code == 200
    alerts = resp.json()
    assert len(alerts) >= 1
    assert alerts[0]["alert_type"] == ALERT_UNKNOWN_DEVICE


def test_integration_rename_triggers_baseline_mismatch(client: TestClient) -> None:
    client.post("/v1/discovery/events", json=_sample_payload())
    device_id = client.get("/v1/devices").json()[0]["device_id"]
    client.patch(f"/v1/devices/{device_id}", json={})

    renamed = _sample_payload()
    renamed["records"][0]["host"] = "renamed-tv.local"
    client.post("/v1/discovery/events", json=renamed)

    resp = client.get("/v1/alerts", params={"alert_type": ALERT_BASELINE_MISMATCH})
    assert resp.status_code == 200
    alerts = resp.json()
    assert len(alerts) == 1


def test_patch_auth_when_token_set(
    client: TestClient, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(settings, "api_token", "secret")
    client.post(
        "/v1/discovery/events",
        json=_sample_payload(),
        headers={"X-API-Key": "secret"},
    )
    device_id = client.get("/v1/devices").json()[0]["device_id"]
    resp = client.patch(f"/v1/devices/{device_id}", json={})
    assert resp.status_code == 401
    resp = client.patch(
        f"/v1/devices/{device_id}",
        json={},
        headers={"X-API-Key": "secret"},
    )
    assert resp.status_code == 200
