"""Deleted task model for trash/recycle bin functionality."""

from __future__ import annotations

import datetime as dt

from sqlalchemy import DateTime, Index
from sqlalchemy.dialects.postgresql import JSONB, UUID
from sqlalchemy.orm import Mapped, mapped_column

from app.models.base import Base, UUIDPrimaryKeyMixin


class DeletedTask(UUIDPrimaryKeyMixin, Base):
    """Stores snapshots of deleted tasks for recovery.

    When a task is deleted, its complete state (including children and time slices)
    is serialized to JSON and stored here. After the retention period expires,
    the record is permanently deleted.
    """

    task_id: Mapped[UUID] = mapped_column(
        UUID(as_uuid=True),
        nullable=False,
        index=True,
        comment="Original task ID",
    )

    deleted_at: Mapped[dt.datetime] = mapped_column(
        DateTime(timezone=True),
        default=lambda: dt.datetime.now(dt.timezone.utc),
        nullable=False,
        comment="When the task was deleted",
    )

    expiry_at: Mapped[dt.datetime] = mapped_column(
        DateTime(timezone=True),
        nullable=False,
        index=True,
        comment="When this record should be permanently deleted",
    )

    snapshot: Mapped[dict] = mapped_column(
        JSONB,
        nullable=False,
        comment="Complete task tree snapshot in JSON format",
    )

    deletion_metadata: Mapped[dict | None] = mapped_column(
        JSONB,
        default=None,
        comment="Additional metadata (deletion reason, stats, etc.)",
    )

    __table_args__ = (
        Index("ix_deleted_task_expiry_at", "expiry_at"),
        Index("ix_deleted_task_task_id", "task_id"),
    )

    def __repr__(self) -> str:  # pragma: no cover - debugging helper
        return f"DeletedTask(id={self.id!s}, task_id={self.task_id!s}, deleted_at={self.deleted_at})"
