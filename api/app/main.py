import sqlite3
from contextlib import asynccontextmanager

from fastapi import Depends, FastAPI, HTTPException, status
from fastapi.responses import JSONResponse

from app import repository
from app.auth import require_api_key
from app.config import settings
from app.db import apply_migrations, connect
from app.models import (
    DiscoveryAcceptedOut,
    DiscoveryEventIn,
    DeviceDetailOut,
    DeviceOut,
    HealthOut,
)


@asynccontextmanager
async def lifespan(_app: FastAPI):
    conn = connect()
    try:
        apply_migrations(conn)
    finally:
        conn.close()
    yield


app = FastAPI(title="LAN Discovery API", lifespan=lifespan)


def get_db() -> sqlite3.Connection:
    conn = connect()
    try:
        yield conn
    finally:
        conn.close()


@app.get("/health", response_model=HealthOut)
def health() -> HealthOut:
    return HealthOut()


@app.post(
    "/v1/discovery/events",
    status_code=status.HTTP_202_ACCEPTED,
    response_model=DiscoveryAcceptedOut,
    dependencies=[Depends(require_api_key)],
)
def ingest_discovery_event(
    event: DiscoveryEventIn,
    conn: sqlite3.Connection = Depends(get_db),
) -> DiscoveryAcceptedOut:
    if not event.records:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="records must not be empty",
        )
    try:
        conn.execute("BEGIN")
        repository.upsert_discovery_event(conn, event)
        conn.commit()
    except ValueError as exc:
        conn.rollback()
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(exc),
        ) from exc
    except sqlite3.Error as exc:
        conn.rollback()
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="database error",
        ) from exc
    return DiscoveryAcceptedOut()


@app.get("/v1/devices", response_model=list[DeviceOut])
def list_devices(conn: sqlite3.Connection = Depends(get_db)) -> list[DeviceOut]:
    try:
        return repository.list_devices(conn)
    except sqlite3.Error as exc:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="database error",
        ) from exc


@app.get("/v1/devices/{device_id}", response_model=DeviceDetailOut)
def get_device(
    device_id: str,
    conn: sqlite3.Connection = Depends(get_db),
) -> DeviceDetailOut:
    try:
        device = repository.get_device(conn, device_id)
    except sqlite3.Error as exc:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="database error",
        ) from exc
    if device is None:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="device not found",
        )
    return device


@app.exception_handler(HTTPException)
async def http_exception_handler(_request, exc: HTTPException):
    return JSONResponse(status_code=exc.status_code, content={"detail": exc.detail})
