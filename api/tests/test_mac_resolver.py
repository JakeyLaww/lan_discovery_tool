from pathlib import Path

from app.mac_resolver import lookup, normalize_mac


def test_normalize_mac_colon() -> None:
    assert normalize_mac("aa:bb:cc:dd:ee:ff") == "AA:BB:CC:DD:EE:FF"


def test_normalize_mac_dash() -> None:
    assert normalize_mac("AA-BB-CC-DD-EE-FF") == "AA:BB:CC:DD:EE:FF"


def test_normalize_mac_invalid() -> None:
    assert normalize_mac("not-a-mac") is None
    assert normalize_mac("") is None


def test_lookup_proc_arp(tmp_path: Path) -> None:
    arp = tmp_path / "arp"
    arp.write_text(
        "IP address       HW type     Flags       HW address            Mask     Device\n"
        "192.168.1.10     0x1         0x2         aa:bb:cc:dd:ee:ff     *        eth0\n"
        "192.168.1.20     0x1         0x2         00:00:00:00:00:00     *        eth0\n",
        encoding="utf-8",
    )
    assert lookup("192.168.1.10", arp_path=arp) == "AA:BB:CC:DD:EE:FF"
    assert lookup("192.168.1.20", arp_path=arp) is None
    assert lookup("192.168.1.99", arp_path=arp) is None
