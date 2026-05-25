from dataclasses import dataclass

from app.mdns_merge import diff_fingerprint, service_sets_equal

ALERT_UNKNOWN_DEVICE = "UNKNOWN_DEVICE"
ALERT_BASELINE_MISMATCH = "BASELINE_MISMATCH"


@dataclass(frozen=True)
class PolicyAlert:
    alert_type: str
    message: str
    dedupe_key: str
    severity: str = "warn"


@dataclass(frozen=True)
class DevicePolicyState:
    device_id: str
    status: str
    mdns_host: str | None
    services: list[str]


@dataclass(frozen=True)
class BaselineState:
    mdns_host: str | None
    services: list[str]


def has_meaningful_identity(mdns_host: str | None, services: list[str]) -> bool:
    return bool(mdns_host) or bool(services)


def evaluate_security_policy(
    device: DevicePolicyState,
    baseline: BaselineState | None,
) -> list[PolicyAlert]:
    alerts: list[PolicyAlert] = []
    if not has_meaningful_identity(device.mdns_host, device.services):
        return alerts

    if device.status == "APPROVED":
        if baseline is None:
            return alerts
        mismatch = _baseline_mismatch_alerts(device, baseline)
        alerts.extend(mismatch)
    else:
        alerts.append(
            PolicyAlert(
                alert_type=ALERT_UNKNOWN_DEVICE,
                message=_unknown_message(device),
                dedupe_key=f"unknown:{device.device_id}",
            )
        )
    return alerts


def _unknown_message(device: DevicePolicyState) -> str:
    host = device.mdns_host or "(no host)"
    n = len(device.services)
    types_hint = ", ".join(device.services[:3])
    if len(device.services) > 3:
        types_hint += ", ..."
    suffix = f" ({types_hint})" if types_hint else ""
    return (
        f"Unknown device: host={host}, service_types={n} (at observation){suffix}"
    )


def _baseline_mismatch_alerts(
    device: DevicePolicyState,
    baseline: BaselineState,
) -> list[PolicyAlert]:
    host_changed = (device.mdns_host or "") != (baseline.mdns_host or "")
    services_changed = not service_sets_equal(device.services, baseline.services)
    if not host_changed and not services_changed:
        return []

    parts: list[str] = []
    if host_changed:
        parts.append(
            f"hostname {baseline.mdns_host!r} -> {device.mdns_host!r}"
        )
    if services_changed:
        parts.append(
            f"service types changed (baseline={sorted(baseline.services)}, "
            f"observed={sorted(device.services)})"
        )

    fingerprint = diff_fingerprint(device.mdns_host, device.services)
    return [
        PolicyAlert(
            alert_type=ALERT_BASELINE_MISMATCH,
            message="Baseline mismatch for approved device: " + "; ".join(parts),
            dedupe_key=f"mismatch:{device.device_id}:{fingerprint}",
        )
    ]
