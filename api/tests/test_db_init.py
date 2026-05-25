import sqlite3
from pathlib import Path

import pytest

from app.db import SCHEMA_VERSION, SchemaVersionError, connect, current_schema_version, ensure_database


def _table_exists(conn: sqlite3.Connection, name: str) -> bool:
    row = conn.execute(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?",
        (name,),
    ).fetchone()
    return row is not None


def test_fresh_db_creates_schema(tmp_path: Path) -> None:
    db_path = tmp_path / "fresh.db"
    conn = connect(db_path)
    ensure_database(conn)
    assert current_schema_version(conn) == SCHEMA_VERSION
    assert _table_exists(conn, "devices")
    assert _table_exists(conn, "device_baselines")
    assert _table_exists(conn, "alerts")
    conn.close()


def test_ensure_database_idempotent(tmp_path: Path) -> None:
    db_path = tmp_path / "idempotent.db"
    conn = connect(db_path)
    ensure_database(conn)
    ensure_database(conn)
    assert current_schema_version(conn) == SCHEMA_VERSION
    conn.close()


def test_wrong_schema_version_raises(tmp_path: Path) -> None:
    db_path = tmp_path / "old.db"
    conn = connect(db_path)
    ensure_database(conn)
    conn.execute(
        "UPDATE schema_version SET version = 0 WHERE version = ?",
        (SCHEMA_VERSION,),
    )
    conn.commit()
    with pytest.raises(SchemaVersionError, match="lan_db.py reset"):
        ensure_database(conn)
    conn.close()
