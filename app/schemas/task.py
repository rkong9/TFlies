"""Pydantic models for task resources."""

from __future__ import annotations

import datetime as dt
from uuid import UUID

from pydantic import Field

from app.models.task import ExecutionState, TaskStatus
from app.schemas.base import APIModel
from app.schemas.time_slice import TimeSliceRead


class TaskBase(APIModel):
    title: str = Field(min_length=1, max_length=200)
    description: str | None = None
    priority: int = Field(ge=1, le=5, default=3)
    due_at: dt.datetime | None = None
    is_ddl: bool = Field(default=False)
    estimated_time_ms: int | None = Field(default=None, ge=0)
    position: float = Field(default=0.0)


class TaskCreate(TaskBase):
    project_id: UUID
    parent_id: UUID | None = None
    status: TaskStatus = TaskStatus.TODO
    execution_state: ExecutionState = ExecutionState.IDLE


class TaskUpdate(APIModel):
    title: str | None = Field(default=None, min_length=1, max_length=200)
    description: str | None = None
    priority: int | None = Field(default=None, ge=1, le=5)
    due_at: dt.datetime | None = None
    is_ddl: bool | None = None
    estimated_time_ms: int | None = Field(default=None, ge=0)
    status: TaskStatus | None = None
    execution_state: ExecutionState | None = None
    parent_id: UUID | None = None


class TaskRead(TaskBase):
    id: UUID
    project_id: UUID
    parent_id: UUID | None
    status: TaskStatus
    execution_state: ExecutionState
    created_at: dt.datetime
    updated_at: dt.datetime
    time_slices: list[TimeSliceRead] = Field(default_factory=list)
    total_logged_ms: int = 0
