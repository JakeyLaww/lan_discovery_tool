from pydantic import BaseModel, Field


class DiscoveryRecordIn(BaseModel):
    owner_name: str
    type: int
    type_name: str | None = None
    rdata_text: str
    ttl: int = 0
    last_seen_ms: int = 0
    service: str | None = None
    host: str | None = None


class DiscoveryEventIn(BaseModel):
    timestamp_ms: int
    src_ip: str
    records: list[DiscoveryRecordIn] = Field(default_factory=list)
    mac: str | None = None


class DiscoveryAcceptedOut(BaseModel):
    status: str = "accepted"


class HealthOut(BaseModel):
    status: str = "ok"


class DeviceOut(BaseModel):
    device_id: str
    src_ip: str
    mdns_host: str | None = None
    mac: str | None = None
    display_name: str | None = None
    status: str
    first_seen_ms: int
    last_seen_ms: int
    services: list[str] = Field(default_factory=list)


class DeviceDetailOut(DeviceOut):
    pass


class DeviceApproveIn(BaseModel):
    display_name: str | None = None


class AlertOut(BaseModel):
    alert_id: str
    device_id: str
    alert_type: str
    message: str
    severity: str
    observed_at_ms: int
    dedupe_key: str
    resolved_at_ms: int | None = None
