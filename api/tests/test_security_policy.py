from app.security_policy import (
    ALERT_BASELINE_MISMATCH,
    ALERT_UNKNOWN_DEVICE,
    BaselineState,
    DevicePolicyState,
    evaluate_security_policy,
    has_meaningful_identity,
)


def test_no_alert_without_meaningful_identity() -> None:
    device = DevicePolicyState(
        device_id="d1",
        status="UNKNOWN",
        mdns_host=None,
        services=[],
    )
    assert not has_meaningful_identity(device.mdns_host, device.services)
    assert evaluate_security_policy(device, None) == []


def test_unknown_device_alert() -> None:
    device = DevicePolicyState(
        device_id="d1",
        status="UNKNOWN",
        mdns_host="tv.local",
        services=["svc._tcp.local"],
    )
    alerts = evaluate_security_policy(device, None)
    assert len(alerts) == 1
    assert alerts[0].alert_type == ALERT_UNKNOWN_DEVICE
    assert alerts[0].dedupe_key == "unknown:d1"
    assert "service_types=1" in alerts[0].message


def test_no_unknown_alert_for_approved() -> None:
    device = DevicePolicyState(
        device_id="d1",
        status="APPROVED",
        mdns_host="tv.local",
        services=[],
    )
    baseline = BaselineState(mdns_host="tv.local", services=[])
    assert evaluate_security_policy(device, baseline) == []


def test_baseline_mismatch_hostname() -> None:
    device = DevicePolicyState(
        device_id="d1",
        status="APPROVED",
        mdns_host="renamed.local",
        services=["svc._tcp.local"],
    )
    baseline = BaselineState(mdns_host="tv.local", services=["svc._tcp.local"])
    alerts = evaluate_security_policy(device, baseline)
    assert len(alerts) == 1
    assert alerts[0].alert_type == ALERT_BASELINE_MISMATCH
    assert alerts[0].dedupe_key.startswith("mismatch:d1:")


def test_baseline_mismatch_services() -> None:
    device = DevicePolicyState(
        device_id="d1",
        status="APPROVED",
        mdns_host="tv.local",
        services=["other._tcp.local"],
    )
    baseline = BaselineState(mdns_host="tv.local", services=["svc._tcp.local"])
    alerts = evaluate_security_policy(device, baseline)
    assert len(alerts) == 1
    assert alerts[0].alert_type == ALERT_BASELINE_MISMATCH
