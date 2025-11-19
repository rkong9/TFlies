"""Shared base schemas."""

from pydantic import BaseModel, ConfigDict


class APIModel(BaseModel):
    """Base class for API schemas with ORM compatibility."""

    model_config = ConfigDict(from_attributes=True, extra="forbid" if __debug__ else "ignore")
