"""Logging configuration built on top of structlog."""

from __future__ import annotations

import logging
from typing import Any, cast

import structlog
from structlog.stdlib import BoundLogger


def _configure_stdlib_logging(level: int) -> None:
    """Configure Python's standard logging module."""

    logging.basicConfig(
        level=level,
        format="%(message)s",
        handlers=[logging.StreamHandler()],
    )


def configure_logging(level: int | str | None = None) -> None:
    """Set up structlog and stdlib logging.

    Args:
        level: Optional logging level; defaults to INFO if not provided.
    """

    resolved_level = logging.getLevelName(level) if isinstance(level, str) else level
    level_value = resolved_level if isinstance(resolved_level, int) else logging.INFO

    _configure_stdlib_logging(level_value)

    structlog.configure(
        processors=[
            structlog.contextvars.merge_contextvars,
            structlog.processors.add_log_level,
            structlog.processors.TimeStamper(fmt="ISO", utc=True),
            structlog.processors.StackInfoRenderer(),
            structlog.processors.format_exc_info,
            structlog.processors.dict_tracebacks,
            structlog.processors.JSONRenderer(),
        ],
        wrapper_class=structlog.make_filtering_bound_logger(level_value),
        cache_logger_on_first_use=True,
    )


def get_logger(*args: Any, **kwargs: Any) -> BoundLogger:
    """Return a configured structlog bound logger."""

    return cast(BoundLogger, structlog.get_logger(*args, **kwargs))
