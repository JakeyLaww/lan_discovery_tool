from app.mdns_names import (
    META_SERVICE,
    is_instance_name,
    is_service_type_name,
    service_type_from_name,
    service_types_from_record,
    service_types_from_records,
)
from app.models import DiscoveryRecordIn


def test_service_type_from_instance() -> None:
    name = "Manasi's 43in Hisense._display._tcp.local"
    assert service_type_from_name(name) == "_display._tcp.local"


def test_meta_ptr_adds_service_type() -> None:
    rec = DiscoveryRecordIn(
        owner_name=META_SERVICE,
        type=12,
        type_name="PTR",
        rdata_text="_display._tcp.local",
        service="_display._tcp.local",
    )
    assert service_types_from_record(rec) == {"_display._tcp.local"}


def test_service_type_ptr_with_instance_rdata() -> None:
    rec = DiscoveryRecordIn(
        owner_name="_display._tcp.local",
        type=12,
        type_name="PTR",
        rdata_text="Manasi's 43in Hisense._display._tcp.local",
        service="Manasi's 43in Hisense._display._tcp.local",
    )
    assert service_types_from_record(rec) == {"_display._tcp.local"}


def test_srv_instance_adds_service_type() -> None:
    rec = DiscoveryRecordIn(
        owner_name="Manasi's 43in Hisense._airplay._tcp.local",
        type=33,
        type_name="SRV",
        rdata_text="target=host.local",
        service="Manasi's 43in Hisense._airplay._tcp.local",
    )
    assert service_types_from_record(rec) == {"_airplay._tcp.local"}


def test_txt_does_not_add_raw_instance_as_type() -> None:
    rec = DiscoveryRecordIn(
        owner_name="Manasi's 43in Hisense._display._tcp.local",
        type=16,
        type_name="TXT",
        rdata_text="container_id=x",
        service="Manasi's 43in Hisense._display._tcp.local",
    )
    assert service_types_from_record(rec) == {"_display._tcp.local"}


def test_batch_dedupes_types() -> None:
    records = [
        DiscoveryRecordIn(
            owner_name=META_SERVICE,
            type=12,
            type_name="PTR",
            rdata_text="_airplay._tcp.local",
            service="_airplay._tcp.local",
        ),
        DiscoveryRecordIn(
            owner_name="Living._airplay._tcp.local",
            type=33,
            type_name="SRV",
            rdata_text="target=tv.local",
            service="Living._airplay._tcp.local",
        ),
    ]
    assert service_types_from_records(records) == ["_airplay._tcp.local"]


def test_is_service_type_name() -> None:
    assert is_service_type_name("_airplay._tcp.local")
    assert not is_service_type_name(META_SERVICE)
    assert is_instance_name("Room._airplay._tcp.local")
