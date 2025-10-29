"""Service layer for project operations."""

from __future__ import annotations

from uuid import UUID

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.models.project import Project
from app.schemas.project import ProjectCreate, ProjectUpdate


class ProjectService:
    """Encapsulates project-related business logic."""

    def __init__(self, session: AsyncSession) -> None:
        self._session = session

    async def create(self, data: ProjectCreate) -> Project:
        project = Project(**data.model_dump())
        self._session.add(project)
        await self._session.flush()
        return project

    async def get(self, project_id: UUID) -> Project | None:
        return await self._session.get(Project, project_id)

    async def list(self) -> list[Project]:
        result = await self._session.execute(select(Project).order_by(Project.created_at.desc()))
        return list(result.scalars())

    async def update(self, project: Project, data: ProjectUpdate) -> Project:
        for field, value in data.model_dump(exclude_unset=True).items():
            setattr(project, field, value)
        await self._session.flush()
        return project

    async def delete(self, project: Project) -> None:
        await self._session.delete(project)
        await self._session.flush()
