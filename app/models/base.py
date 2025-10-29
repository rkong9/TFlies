"""Base classes shared by ORM models."""

from __future__ import annotations

import datetime as dt
import uuid

from sqlalchemy import DateTime, MetaData
from sqlalchemy.orm import DeclarativeBase, Mapped, declared_attr, mapped_column

metadata = MetaData()


class Base(DeclarativeBase):
    """Declarative base class with naming conventions."""

    metadata = metadata

    @declared_attr.directive
    def __tablename__(cls) -> str:  # noqa: N805 - SQLAlchemy uses class for table naming
        return cls.__name__.lower()


class TimestampMixin:
    """Mixin that adds created/updated timestamp columns.

    All timestamps are stored as UTC in the database (timestamp with time zone).
    Default values use timezone-aware UTC datetime objects.
    """

    created_at: Mapped[dt.datetime] = mapped_column(
        DateTime(timezone=True),
        default=lambda: dt.datetime.now(dt.timezone.utc),
        nullable=False,
    )
    updated_at: Mapped[dt.datetime] = mapped_column(
        DateTime(timezone=True),
        default=lambda: dt.datetime.now(dt.timezone.utc),
        onupdate=lambda: dt.datetime.now(dt.timezone.utc),
        nullable=False,
    )


class UUIDPrimaryKeyMixin:
    """Mixin providing a UUID primary key column."""

    id: Mapped[uuid.UUID] = mapped_column(primary_key=True, default=uuid.uuid4)
