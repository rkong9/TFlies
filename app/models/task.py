"""Task domain model."""

from __future__ import annotations

import datetime as dt
import enum
import uuid
from typing import TYPE_CHECKING

from sqlalchemy import BigInteger, Boolean, CheckConstraint, DateTime, Enum, Float, ForeignKey, Integer, String, Text
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.models.base import Base, TimestampMixin, UUIDPrimaryKeyMixin

if TYPE_CHECKING:  # pragma: no cover
    from app.models.project import Project
    from app.models.time_slice import TimeSlice


class TaskStatus(str, enum.Enum):
    """Lifecycle states for a task."""

    TODO = "todo"
    IN_PROGRESS = "in_progress"
    SUSPENDED = "suspended"
    COMPLETED = "completed"


class ExecutionState(str, enum.Enum):
    """Execution states for a task (only valid when status is IN_PROGRESS)."""

    IDLE = "idle"
    WORKING = "working"


class Task(UUIDPrimaryKeyMixin, TimestampMixin, Base):
    """Represents a task belonging to a project, optionally nested."""

    title: Mapped[str] = mapped_column(String(200), nullable=False)
    description: Mapped[str | None] = mapped_column(Text(), default=None)

    priority: Mapped[int] = mapped_column(Integer, nullable=False, default=3)
    due_at: Mapped[dt.datetime | None] = mapped_column(DateTime(timezone=True), default=None)
    is_ddl: Mapped[bool] = mapped_column(
        Boolean,
        nullable=False,
        default=False,
        comment="If True, task becomes locked and read-only after due_at expires",
    )
    estimated_time_ms: Mapped[int | None] = mapped_column(BigInteger, nullable=True, default=None)
    status: Mapped[TaskStatus] = mapped_column(
        Enum(TaskStatus, name="task_status"),
        nullable=False,
        default=TaskStatus.TODO,
    )
    execution_state: Mapped[ExecutionState] = mapped_column(
        Enum(ExecutionState, name="execution_state"),
        nullable=False,
        default=ExecutionState.IDLE,
    )

    position: Mapped[float] = mapped_column(
        Float,
        nullable=False,
        default=0.0,
        index=True,
        comment="Sort position for ordering tasks within the same parent",
    )

    project_id: Mapped[uuid.UUID] = mapped_column(
        ForeignKey("project.id", ondelete="CASCADE"),
        nullable=False,
    )
    parent_id: Mapped[uuid.UUID | None] = mapped_column(
        ForeignKey("task.id", ondelete="CASCADE"),
        default=None,
    )

    project: Mapped[Project] = relationship(back_populates="tasks", lazy="joined")
    parent: Mapped[Task | None] = relationship(
        remote_side="Task.id",
        back_populates="children",
        lazy="selectin",
    )
    children: Mapped[list[Task]] = relationship(
        back_populates="parent",
        cascade="all, delete-orphan",
        lazy="selectin",
    )
    time_slices: Mapped[list[TimeSlice]] = relationship(
        back_populates="task",
        cascade="all, delete-orphan",
        lazy="selectin",
    )

    __table_args__ = (
        CheckConstraint(
            "(status = 'in_progress') OR (execution_state = 'idle')",
            name="ck_execution_only_when_in_progress",
        ),
    )

    @property
    def total_logged_ms(self) -> int:
        """Calculate total logged time from all time slices."""
        # 只计算已完成的时间片（duration_ms 不为 None）
        return sum(ts.duration_ms for ts in self.time_slices if ts.duration_ms is not None)

    def __repr__(self) -> str:  # pragma: no cover - debugging helper
        return f"Task(id={self.id!s}, title={self.title!r}, status={self.status})"
