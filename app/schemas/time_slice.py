"""Pydantic models for time slice resources."""

from __future__ import annotations

import datetime as dt
from typing import Optional
from uuid import UUID

from pydantic import Field

from app.schemas.base import APIModel


class TimeSliceBase(APIModel):
    start_at: dt.datetime
    end_at: Optional[dt.datetime] = None
    efficiency_score: Optional[int] = Field(default=None, ge=1, le=5)
    note: Optional[str] = None


class TimeSliceCreate(APIModel):
    task_id: UUID
    start_at: dt.datetime
    end_at: dt.datetime | None = None
    efficiency_score: int | None = None
    note: str | None = None
    duration_ms: int | None = None


class TimeSliceUpdate(APIModel):
    start_at: dt.datetime | None = None
    end_at: dt.datetime | None = None
    efficiency_score: int | None = Field(default=None, ge=1, le=5)
    note: str | None = None
    duration_ms: int | None = Field(default=None, ge=0)


class TimeSliceRead(TimeSliceBase):
    id: UUID
    task_id: UUID
    duration_ms: int | None
    created_at: dt.datetime
    updated_at: dt.datetime
