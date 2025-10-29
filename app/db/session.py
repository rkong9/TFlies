"""Database session management utilities."""

from __future__ import annotations

from collections.abc import AsyncGenerator

from sqlalchemy.ext.asyncio import (
    AsyncEngine,
    AsyncSession,
    async_sessionmaker,
    create_async_engine,
)

from app.core.config import settings

_engine: AsyncEngine | None = None
_session_factory: async_sessionmaker[AsyncSession] | None = None


def get_engine() -> AsyncEngine:
    """Return a singleton async engine instance."""

    global _engine

    if _engine is None:
        _engine = create_async_engine(
            settings.database_url,
            echo=settings.debug,
            pool_pre_ping=True,
        )

    return _engine


def get_session_factory() -> async_sessionmaker[AsyncSession]:
    """Return a lazily created async session factory."""

    global _session_factory

    if _session_factory is None:
        _session_factory = async_sessionmaker(get_engine(), expire_on_commit=False)

    return _session_factory


async def get_session() -> AsyncGenerator[AsyncSession, None]:
    """Yield an async SQLAlchemy session for dependency injection."""

    factory = get_session_factory()
    async with factory() as session:
        yield session
