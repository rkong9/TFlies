"""Project domain model."""

from __future__ import annotations

import datetime as dt
from typing import TYPE_CHECKING

from sqlalchemy import BigInteger, DateTime, String, Text
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.models.base import Base, TimestampMixin, UUIDPrimaryKeyMixin

if TYPE_CHECKING:  # pragma: no cover
    from app.models.task import Task


class Project(UUIDPrimaryKeyMixin, TimestampMixin, Base):
    """Represents a project grouping multiple tasks."""

    name: Mapped[str] = mapped_column(String(120), nullable=False, unique=True)
    description: Mapped[str | None] = mapped_column(Text(), default=None)

    planned_time_ms: Mapped[int] = mapped_column(BigInteger, nullable=False, default=0)
    start_at: Mapped[dt.datetime | None] = mapped_column(DateTime(timezone=True), default=None)
    end_at: Mapped[dt.datetime | None] = mapped_column(DateTime(timezone=True), default=None)

    tasks: Mapped[list[Task]] = relationship(
        back_populates="project",
        cascade="all, delete-orphan",
        lazy="selectin",
    )

    @property
    def total_logged_ms(self) -> int:
        """Calculate total logged time from all tasks."""
        return sum(task.total_logged_ms for task in self.tasks)

    def __repr__(self) -> str:  # pragma: no cover - for debugging convenience
        return f"Project(id={self.id!s}, name={self.name!r})"
