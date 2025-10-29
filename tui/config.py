"""Configuration helpers for the TUI client."""

from __future__ import annotations

import os
from dataclasses import dataclass

DEFAULT_API_URL = "http://127.0.0.1:8000/api"
DEFAULT_API_TIMEOUT = 10.0
DEFAULT_REFRESH_INTERVAL = 30.0
DEFAULT_TRUST_ENV_PROXIES = False


@dataclass(slots=True)
class TUIConfig:
    """Runtime configuration values consumed by the TUI."""

    api_base_url: str = DEFAULT_API_URL
    request_timeout: float = DEFAULT_API_TIMEOUT
    refresh_interval: float = DEFAULT_REFRESH_INTERVAL
    trust_env_proxies: bool = DEFAULT_TRUST_ENV_PROXIES


def load_config() -> TUIConfig:
    """Load configuration from environment variables with sensible defaults."""

    def _bool_env(name: str, default: bool) -> bool:
        raw = os.getenv(name)
        if raw is None:
            return default
        return raw.strip().lower() in {"1", "true", "yes", "on"}

    return TUIConfig(
        api_base_url=os.getenv("TIMEBOX_API_URL", DEFAULT_API_URL),
        request_timeout=float(os.getenv("TIMEBOX_API_TIMEOUT", DEFAULT_API_TIMEOUT)),
        refresh_interval=float(os.getenv("TIMEBOX_REFRESH_INTERVAL", DEFAULT_REFRESH_INTERVAL)),
        trust_env_proxies=_bool_env("TIMEBOX_TRUST_ENV_PROXIES", DEFAULT_TRUST_ENV_PROXIES),
    )


__all__ = ["TUIConfig", "load_config"]
