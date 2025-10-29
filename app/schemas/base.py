"""Shared base schemas."""

from __future__ import annotations

from pydantic import BaseModel, ConfigDict


class APIModel(BaseModel):
    """Base class for API schemas with ORM compatibility."""

    model_config = ConfigDict(from_attributes=True, extra="forbid" if __debug__ else "ignore")
