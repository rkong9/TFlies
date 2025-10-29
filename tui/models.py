"""Data transfer objects used by the TUI client."""

from __future__ import annotations

import datetime as dt
from dataclasses import dataclass, field
from typing import Any
from uuid import UUID


def _parse_datetime(value: str | None) -> dt.datetime | None:
    if value is None:
        return None
    try:
        return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None


@dataclass(slots=True)
class TimeSliceDTO:
    id: UUID
    task_id: UUID
    start_at: dt.datetime | None
    end_at: dt.datetime | None
    duration_ms: int
    efficiency_score: int
    note: str | None
    created_at: dt.datetime | None
    updated_at: dt.datetime | None

    @classmethod
    def from_api(cls, payload: dict[str, Any]) -> TimeSliceDTO:
        return cls(
            id=UUID(payload["id"]),
            task_id=UUID(payload["task_id"]),
            start_at=_parse_datetime(payload.get("start_at")),
            end_at=_parse_datetime(payload.get("end_at")),
            duration_ms=int(payload.get("duration_ms", 0)),
            efficiency_score=int(payload.get("efficiency_score", 0)),
            note=payload.get("note"),
            created_at=_parse_datetime(payload.get("created_at")),
            updated_at=_parse_datetime(payload.get("updated_at")),
        )


@dataclass(slots=True)
class TaskDTO:
    id: UUID
    project_id: UUID
    title: str
    description: str | None
    priority: int
    due_at: dt.datetime | None
    estimated_time_ms: int
    status: str
    parent_id: UUID | None
    created_at: dt.datetime | None
    updated_at: dt.datetime | None
    time_slices: list[TimeSliceDTO] = field(default_factory=list)

    @classmethod
    def from_api(cls, payload: dict[str, Any]) -> TaskDTO:
        time_slices = [TimeSliceDTO.from_api(item) for item in payload.get("time_slices", [])]
        return cls(
            id=UUID(payload["id"]),
            project_id=UUID(payload["project_id"]),
            title=payload.get("title", "Unnamed Task"),
            description=payload.get("description"),
            priority=int(payload.get("priority", 0)),
            due_at=_parse_datetime(payload.get("due_at")),
            estimated_time_ms=int(payload.get("estimated_time_ms", 0)),
            status=str(payload.get("status", "unknown")),
            parent_id=UUID(payload["parent_id"]) if payload.get("parent_id") else None,
            created_at=_parse_datetime(payload.get("created_at")),
            updated_at=_parse_datetime(payload.get("updated_at")),
            time_slices=time_slices,
        )

    @property
    def is_completed(self) -> bool:
        return self.status == "completed"

    @property
    def total_logged_ms(self) -> int:
        return sum(slice.duration_ms for slice in self.time_slices)


@dataclass(slots=True)
class ProjectDTO:
    id: UUID
    name: str
    description: str | None
    planned_time_ms: int
    start_at: dt.datetime | None
    end_at: dt.datetime | None
    created_at: dt.datetime | None
    updated_at: dt.datetime | None
    tasks: list[TaskDTO] = field(default_factory=list)

    @classmethod
    def from_api(cls, payload: dict[str, Any], tasks: list[TaskDTO]) -> ProjectDTO:
        return cls(
            id=UUID(payload["id"]),
            name=payload.get("name", "Unnamed Project"),
            description=payload.get("description"),
            planned_time_ms=int(payload.get("planned_time_ms", 0)),
            start_at=_parse_datetime(payload.get("start_at")),
            end_at=_parse_datetime(payload.get("end_at")),
            created_at=_parse_datetime(payload.get("created_at")),
            updated_at=_parse_datetime(payload.get("updated_at")),
            tasks=tasks,
        )

    @property
    def total_logged_ms(self) -> int:
        return sum(task.total_logged_ms for task in self.tasks)


__all__ = ["ProjectDTO", "TaskDTO", "TimeSliceDTO"]
