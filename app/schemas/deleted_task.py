"""Pydantic models for deleted task/trash resources."""

import datetime as dt
from uuid import UUID

from app.schemas.base import APIModel


class DeletedTaskRead(APIModel):
    """Response model for deleted tasks in trash."""

    id: UUID
    task_id: UUID
    deleted_at: dt.datetime
    expiry_at: dt.datetime
    snapshot: dict
    deletion_metadata: dict | None

    # Convenience fields from deletion_metadata
    @property
    def title(self) -> str:
        """Extract task title from deletion_metadata."""
        return self.deletion_metadata.get("title", "Unknown") if self.deletion_metadata else "Unknown"

    @property
    def project_id(self) -> str:
        """Extract project ID from deletion_metadata."""
        return self.deletion_metadata.get("project_id", "") if self.deletion_metadata else ""

    @property
    def total_descendants(self) -> int:
        """Extract total descendants count from deletion_metadata."""
        return self.deletion_metadata.get("total_descendants", 1) if self.deletion_metadata else 1


class DeletedTaskSummary(APIModel):
    """Summary model for deleted tasks (without full snapshot)."""

    id: UUID
    task_id: UUID
    deleted_at: dt.datetime
    expiry_at: dt.datetime
    title: str
    project_id: str
    total_descendants: int


class TrashStatsResponse(APIModel):
    """Statistics about trash contents."""

    total_items: int
    total_tasks: int  # Including all descendants
    oldest_item: dt.datetime | None
    newest_item: dt.datetime | None
