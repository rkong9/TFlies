"""Time slice domain model."""

from __future__ import annotations

import datetime as dt
import uuid
from typing import TYPE_CHECKING

from sqlalchemy import BigInteger, CheckConstraint, DateTime, ForeignKey, Integer, Text
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.models.base import Base, TimestampMixin, UUIDPrimaryKeyMixin

if TYPE_CHECKING:  # pragma: no cover
    from app.models.task import Task


class TimeSlice(UUIDPrimaryKeyMixin, TimestampMixin, Base):
    """Represents a contiguous period of task execution."""

    task_id: Mapped[uuid.UUID] = mapped_column(
        ForeignKey("task.id", ondelete="CASCADE"),
        nullable=False,
    )

    start_at: Mapped[dt.datetime] = mapped_column(DateTime(timezone=True), nullable=False)
    end_at: Mapped[dt.datetime | None] = mapped_column(DateTime(timezone=True), nullable=True, default=None)

    duration_ms: Mapped[int | None] = mapped_column(BigInteger, nullable=True, default=None)
    efficiency_score: Mapped[int | None] = mapped_column(Integer, nullable=True, default=None)
    note: Mapped[str | None] = mapped_column(Text(), default=None)

    task: Mapped[Task] = relationship(back_populates="time_slices", lazy="joined")

    __table_args__ = (
        CheckConstraint("duration_ms IS NULL OR duration_ms >= 0", name="ck_time_slice_duration_positive"),
        CheckConstraint("efficiency_score IS NULL OR efficiency_score BETWEEN 1 AND 5", name="ck_time_slice_efficiency_range"),
        CheckConstraint("end_at IS NULL OR end_at >= start_at", name="ck_time_slice_temporal_order"),
    )

    def __repr__(self) -> str:  # pragma: no cover - debugging helper
        return f"TimeSlice(id={self.id!s}, task={self.task_id!s}, duration_ms={self.duration_ms})"
