import sqlite3
from pathlib import Path

from app.config import settings

# Bump when schema.sql changes; reset DB with: python3 scripts/lan_db.py reset
SCHEMA_VERSION = 1


class SchemaVersionError(RuntimeError):
    """Database schema does not match this API version."""


def connect(db_path: Path | None = None) -> sqlite3.Connection:
    path = db_path if db_path is not None else settings.db_path
    path.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(path, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA foreign_keys=ON")
    return conn


def current_schema_version(conn: sqlite3.Connection) -> int | None:
    try:
        row = conn.execute(
            "SELECT version FROM schema_version ORDER BY version DESC LIMIT 1"
        ).fetchone()
    except sqlite3.OperationalError:
        return None
    return int(row["version"]) if row else None


def _schema_sql_path() -> Path:
    return settings.migrations_dir / "schema.sql"


def ensure_database(conn: sqlite3.Connection) -> None:
    version = current_schema_version(conn)
    if version is None:
        sql_path = _schema_sql_path()
        conn.executescript(sql_path.read_text(encoding="utf-8"))
        conn.execute(
            "INSERT INTO schema_version (version) VALUES (?)",
            (SCHEMA_VERSION,),
        )
        conn.commit()
        return

    if version != SCHEMA_VERSION:
        raise SchemaVersionError(
            f"Database schema version {version} does not match API "
            f"(expected {SCHEMA_VERSION}). Stop the API and run: "
            "python3 scripts/lan_db.py reset"
        )
