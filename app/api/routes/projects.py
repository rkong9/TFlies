"""Project API endpoints."""

from __future__ import annotations

from uuid import UUID

from fastapi import APIRouter, HTTPException, status

from app.api.dependencies import DatabaseSession
from app.schemas.project import ProjectCreate, ProjectRead, ProjectUpdate
from app.services.project_service import ProjectService

router = APIRouter(prefix="/projects", tags=["Projects"])


@router.post("/", response_model=ProjectRead, status_code=status.HTTP_201_CREATED)
async def create_project(payload: ProjectCreate, session: DatabaseSession) -> ProjectRead:
    service = ProjectService(session)
    try:
        project = await service.create(payload)
        await session.commit()
        await session.refresh(project)
        return ProjectRead.model_validate(project)
    except Exception:
        await session.rollback()
        raise


@router.get("/", response_model=list[ProjectRead])
async def list_projects(session: DatabaseSession) -> list[ProjectRead]:
    service = ProjectService(session)
    projects = await service.list()
    # 确保所有任务的time_slices被加载
    for project in projects:
        await session.refresh(project, attribute_names=["tasks"])
        for task in project.tasks:
            await session.refresh(task, attribute_names=["time_slices"])
    return [ProjectRead.model_validate(project) for project in projects]


@router.get("/{project_id}", response_model=ProjectRead)
async def get_project(project_id: UUID, session: DatabaseSession) -> ProjectRead:
    service = ProjectService(session)
    project = await service.get(project_id)
    if not project:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Project not found")
    return ProjectRead.model_validate(project)


@router.patch("/{project_id}", response_model=ProjectRead)
async def update_project(
    project_id: UUID,
    payload: ProjectUpdate,
    session: DatabaseSession,
) -> ProjectRead:
    service = ProjectService(session)
    project = await service.get(project_id)
    if not project:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Project not found")
    try:
        await service.update(project, payload)
        await session.commit()
        await session.refresh(project)
        return ProjectRead.model_validate(project)
    except Exception:
        await session.rollback()
        raise


@router.delete("/{project_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_project(project_id: UUID, session: DatabaseSession) -> None:
    service = ProjectService(session)
    project = await service.get(project_id)
    if not project:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Project not found")
    try:
        await service.delete(project)
        await session.commit()
    except Exception:
        await session.rollback()
        raise
