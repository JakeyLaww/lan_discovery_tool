#!/usr/bin/env python3
"""Dump or reset the LAN Discovery SQLite database (stdlib only)."""

from __future__ import annotations

import argparse
import os
import sqlite3
import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parent.parent
_DEFAULT_DB = _REPO_ROOT / "api" / "data" / "lan.db"
_PAYLOAD_PREVIEW_LEN = 200

_TABLES = ("schema_version", "devices", "services", "discovery_events")


def resolve_db_path(explicit: Path | None) -> Path:
    if explicit is not None:
        return explicit.expanduser().resolve()
    env = os.environ.get("LAN_DB_PATH")
    if env:
        return Path(env).expanduser().resolve()
    return _DEFAULT_DB.resolve()


def _wal_paths(db_path: Path) -> list[Path]:
    return [db_path, Path(f"{db_path}-wal"), Path(f"{db_path}-shm")]


def cmd_dump(db_path: Path) -> int:
    if not db_path.is_file():
        print(f"No database at {db_path}")
        return 0

    conn = sqlite3.connect(f"file:{db_path}?mode=ro", uri=True)
    conn.row_factory = sqlite3.Row
    try:
        for table in _TABLES:
            try:
                count = conn.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]
            except sqlite3.OperationalError:
                print(f"\n=== {table} (missing) ===")
                continue

            print(f"\n=== {table} ({count} rows) ===")
            if count == 0:
                continue

            rows = conn.execute(f"SELECT * FROM {table}").fetchall()
            for row in rows:
                data = dict(row)
                if table == "discovery_events" and "payload_json" in data:
                    payload = data["payload_json"]
                    if isinstance(payload, str) and len(payload) > _PAYLOAD_PREVIEW_LEN:
                        data["payload_json"] = (
                            payload[:_PAYLOAD_PREVIEW_LEN] + "… (truncated)"
                        )
                print(data)
    finally:
        conn.close()
    return 0


def cmd_reset(db_path: Path) -> int:
    removed: list[Path] = []
    for path in _wal_paths(db_path):
        if path.is_file():
            path.unlink()
            removed.append(path)

    if not removed:
        print(f"No database files at {db_path} (nothing to reset)")
        return 0

    print("Removed:")
    for path in removed:
        print(f"  {path}")
    print(
        "Note: stop uvicorn if reset fails (file in use). "
        "The API recreates the schema on next startup."
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command",
        choices=("dump", "reset"),
        help="dump: print tables; reset: delete DB files for a fresh start",
    )
    parser.add_argument(
        "--db-path",
        type=Path,
        default=None,
        help="SQLite file (default: LAN_DB_PATH or api/data/lan.db)",
    )
    args = parser.parse_args()
    db_path = resolve_db_path(args.db_path)

    if args.command == "dump":
        return cmd_dump(db_path)
    return cmd_reset(db_path)


if __name__ == "__main__":
    sys.exit(main())
