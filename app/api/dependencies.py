"""Shared dependency providers for the API layer."""

from __future__ import annotations

from typing import Annotated

from fastapi import Depends
from sqlalchemy.ext.asyncio import AsyncSession

from app.db.session import get_session

DatabaseSession = Annotated[AsyncSession, Depends(get_session)]

__all__ = ["DatabaseSession"]
