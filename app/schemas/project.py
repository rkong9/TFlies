"""Pydantic models for project resources."""

from __future__ import annotations

import datetime as dt
from uuid import UUID

from pydantic import Field

from app.schemas.base import APIModel


class ProjectBase(APIModel):
    name: str = Field(min_length=1, max_length=120)
    description: str | None = None
    planned_time_ms: int = Field(ge=0)
    start_at: dt.datetime | None = None
    end_at: dt.datetime | None = None


class ProjectCreate(ProjectBase):
    pass


class ProjectUpdate(APIModel):
    name: str | None = Field(default=None, min_length=1, max_length=120)
    description: str | None = None
    planned_time_ms: int | None = Field(default=None, ge=0)
    start_at: dt.datetime | None = None
    end_at: dt.datetime | None = None


class ProjectRead(ProjectBase):
    id: UUID
    created_at: dt.datetime
    updated_at: dt.datetime
    tasks: list["TaskRead"] = []
    total_logged_ms: int = 0


# Import TaskRead after defining ProjectRead to avoid circular import
from app.schemas.task import TaskRead  # noqa: E402

ProjectRead.model_rebuild()
