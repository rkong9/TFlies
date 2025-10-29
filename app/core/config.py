"""Application configuration utilities."""

from __future__ import annotations

from functools import lru_cache
from typing import Literal

from pydantic_settings import BaseSettings, SettingsConfigDict


class AppSettings(BaseSettings):
    """Sources and exposes application-level configuration."""

    model_config = SettingsConfigDict(env_file=(".env", ".env.local"), env_file_encoding="utf-8")

    app_name: str = "TBox"
    environment: Literal["development", "staging", "production", "test"] = "development"
    debug: bool = False
    api_prefix: str = "/api"
    version: str = "0.1.0"

    database_url: str = "postgresql+asyncpg://tf_user:tf_user@localhost:5432/timebox"
    redis_url: str = "redis://localhost:6379/0"

    cors_allow_origins: list[str] = ["*"]
    cors_allow_credentials: bool = True
    cors_allow_methods: list[str] = ["*"]
    cors_allow_headers: list[str] = ["*"]

    # Trash/Recycle bin settings
    trash_retention_days: int = 30  # Days before permanently deleting tasks from trash


@lru_cache
def get_settings() -> AppSettings:
    """Return a cached settings instance."""

    return AppSettings()


settings = get_settings()
