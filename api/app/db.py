import sqlite3
from pathlib import Path

from app.config import settings


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


def apply_migrations(conn: sqlite3.Connection, migrations_dir: Path | None = None) -> None:
    mig_dir = migrations_dir if migrations_dir is not None else settings.migrations_dir
    version = current_schema_version(conn)
    if version is not None and version >= 1:
        return

    sql_path = mig_dir / "001_init.sql"
    conn.executescript(sql_path.read_text(encoding="utf-8"))
    conn.execute(
        "INSERT OR REPLACE INTO schema_version (version) VALUES (?)",
        (1,),
    )
    conn.commit()
