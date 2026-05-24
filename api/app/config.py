from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict

_API_ROOT = Path(__file__).resolve().parent.parent


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_prefix="LAN_", extra="ignore")

    db_path: Path = _API_ROOT / "data" / "lan.db"
    api_token: str | None = None
    host: str = "127.0.0.1"
    port: int = 8000

    @property
    def migrations_dir(self) -> Path:
        return _API_ROOT / "migrations"


settings = Settings()
