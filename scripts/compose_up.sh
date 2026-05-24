#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPOSE_FILE="$ROOT/deploy/docker-compose.yml"
PROJECT_NAME="${LAN_COMPOSE_PROJECT:-deploy}"

usage() {
    echo "Usage: $0 [--reset-db] [docker compose up args...]"
    echo "  (default)  docker compose up --build"
    echo "  --reset-db Stop stack, remove SQLite volume, then up --build"
}

RESET_DB=0
COMPOSE_ARGS=()

for arg in "$@"; do
    case "$arg" in
        --reset-db) RESET_DB=1 ;;
        -h|--help) usage; exit 0 ;;
        *) COMPOSE_ARGS+=("$arg") ;;
    esac
done

if [[ ${#COMPOSE_ARGS[@]} -eq 0 ]]; then
    COMPOSE_ARGS=(up --build)
fi

compose() {
    docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" "$@"
}

if [[ "$RESET_DB" -eq 1 ]]; then
    echo "Stopping compose stack and removing SQLite volume..."
    compose down --remove-orphans 2>/dev/null || true
    VOLUME_NAME="${PROJECT_NAME}_lan_db"
    if docker volume inspect "$VOLUME_NAME" &>/dev/null; then
        docker volume rm "$VOLUME_NAME"
        echo "Removed volume $VOLUME_NAME"
    else
        echo "Volume $VOLUME_NAME not found (already clean)"
    fi
fi

exec compose "${COMPOSE_ARGS[@]}"
