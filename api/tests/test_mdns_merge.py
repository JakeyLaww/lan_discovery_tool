from app.mdns_merge import diff_fingerprint, service_sets_equal


def test_service_sets_equal_order_insensitive() -> None:
    assert service_sets_equal(
        ["_raop._tcp.local", "_airplay._tcp.local"],
        ["_airplay._tcp.local", "_raop._tcp.local"],
    )


def test_service_sets_equal_detects_diff() -> None:
    assert not service_sets_equal(
        ["_airplay._tcp.local"],
        ["_raop._tcp.local"],
    )


def test_diff_fingerprint_stable_for_same_identity() -> None:
    a = diff_fingerprint("tv.local", ["_airplay._tcp.local"])
    b = diff_fingerprint("tv.local", ["_airplay._tcp.local"])
    assert a == b
    assert len(a) == 16


def test_diff_fingerprint_changes_with_host_or_services() -> None:
    base = diff_fingerprint("tv.local", ["_airplay._tcp.local"])
    assert diff_fingerprint("other.local", ["_airplay._tcp.local"]) != base
    assert diff_fingerprint("tv.local", ["_raop._tcp.local"]) != base
    assert diff_fingerprint(None, ["_airplay._tcp.local"]) != base
