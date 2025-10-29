"""Task API endpoints."""

from __future__ import annotations

from typing import Annotated
from uuid import UUID

from fastapi import APIRouter, HTTPException, Query, status

from app.api.dependencies import DatabaseSession
from app.schemas.deleted_task import DeletedTaskRead, DeletedTaskSummary
from app.schemas.task import TaskCreate, TaskRead, TaskUpdate
from app.services.task_service import TaskService

router = APIRouter(prefix="/tasks", tags=["Tasks"])


@router.post("/", response_model=TaskRead, status_code=status.HTTP_201_CREATED)
async def create_task(payload: TaskCreate, session: DatabaseSession) -> TaskRead:
    service = TaskService(session)
    try:
        task = await service.create(payload)
        await session.commit()
        await session.refresh(task, attribute_names=["time_slices"])
        return TaskRead.model_validate(task)
    except Exception:
        await session.rollback()
        raise


@router.get("/", response_model=list[TaskRead])
async def list_tasks(
    session: DatabaseSession,
    project_id: Annotated[UUID | None, Query()] = None,
) -> list[TaskRead]:
    service = TaskService(session)
    tasks = await (service.list_by_project(project_id) if project_id else service.list())
    # 确保time_slices被加载，以便正确计算total_logged_ms
    for task in tasks:
        await session.refresh(task, attribute_names=["time_slices"])
    return [TaskRead.model_validate(task) for task in tasks]


@router.get("/{task_id}", response_model=TaskRead)
async def get_task(task_id: UUID, session: DatabaseSession) -> TaskRead:
    service = TaskService(session)
    task = await service.get(task_id)
    if not task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Task not found")
    await session.refresh(task, attribute_names=["time_slices"])
    return TaskRead.model_validate(task)


@router.patch("/{task_id}", response_model=TaskRead)
async def update_task(
    task_id: UUID,
    payload: TaskUpdate,
    session: DatabaseSession,
) -> TaskRead:
    service = TaskService(session)
    task = await service.get(task_id)
    if not task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Task not found")
    try:
        await service.update(task, payload)
        await session.commit()
        await session.refresh(task, attribute_names=["time_slices"])
        return TaskRead.model_validate(task)
    except Exception:
        await session.rollback()
        raise


@router.delete("/{task_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_task(task_id: UUID, session: DatabaseSession) -> None:
    service = TaskService(session)
    task = await service.get(task_id)
    if not task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Task not found")
    try:
        await service.delete(task)
        await session.commit()
    except Exception:
        await session.rollback()
        raise


@router.patch("/{task_id}/position", response_model=TaskRead)
async def update_task_position(
    task_id: UUID,
    new_position: Annotated[float, Query()],
    session: DatabaseSession,
) -> TaskRead:
    """Update task position for drag-and-drop sorting."""
    import math

    # 验证 position 是有效的数值
    if math.isnan(new_position) or math.isinf(new_position):
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Invalid position value: must be a finite number"
        )

    service = TaskService(session)
    task = await service.get(task_id)
    if not task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Task not found")
    try:
        await service.update_position(task, new_position)
        await session.commit()
        await session.refresh(task, attribute_names=["time_slices"])
        return TaskRead.model_validate(task)
    except Exception:
        await session.rollback()
        raise


@router.post("/{task_id}/unlock", response_model=TaskRead)
async def unlock_task(task_id: UUID, session: DatabaseSession) -> TaskRead:
    """Unlock a DDL-locked task by disabling its DDL mode.

    This allows the task to be modified even if the deadline has passed.
    Only affects the current task, not its parent or children.
    """
    service = TaskService(session)
    task = await service.get(task_id)
    if not task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Task not found")

    try:
        await service.unlock_task(task)
        await session.commit()
        await session.refresh(task, attribute_names=["time_slices"])
        return TaskRead.model_validate(task)
    except Exception:
        await session.rollback()
        raise


# Trash/Recycle bin endpoints

@router.get("/trash/list", response_model=list[DeletedTaskSummary])
async def list_trash(session: DatabaseSession) -> list[DeletedTaskSummary]:
    """List all tasks in trash (soft-deleted tasks)."""
    service = TaskService(session)
    deleted_tasks = await service.list_trash()

    return [
        DeletedTaskSummary(
            id=dt.id,
            task_id=dt.task_id,
            deleted_at=dt.deleted_at,
            expiry_at=dt.expiry_at,
            title=dt.deletion_metadata.get("title", "Unknown") if dt.deletion_metadata else "Unknown",
            project_id=dt.deletion_metadata.get("project_id", "") if dt.deletion_metadata else "",
            total_descendants=dt.deletion_metadata.get("total_descendants", 1) if dt.deletion_metadata else 1,
        )
        for dt in deleted_tasks
    ]


@router.get("/trash/{deleted_task_id}", response_model=DeletedTaskRead)
async def get_deleted_task(deleted_task_id: UUID, session: DatabaseSession) -> DeletedTaskRead:
    """Get details of a specific deleted task including full snapshot."""
    service = TaskService(session)
    deleted_task = await service.get_deleted_task(deleted_task_id)
    if not deleted_task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Deleted task not found")
    return DeletedTaskRead.model_validate(deleted_task)


@router.post("/trash/{deleted_task_id}/restore", response_model=TaskRead, status_code=status.HTTP_200_OK)
async def restore_task(deleted_task_id: UUID, session: DatabaseSession) -> TaskRead:
    """Restore a task from trash."""
    service = TaskService(session)
    deleted_task = await service.get_deleted_task(deleted_task_id)
    if not deleted_task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Deleted task not found")

    try:
        restored_task = await service.restore_from_trash(deleted_task)
        await session.commit()
        await session.refresh(restored_task, attribute_names=["time_slices"])
        return TaskRead.model_validate(restored_task)
    except Exception:
        await session.rollback()
        raise


@router.delete("/trash/{deleted_task_id}", status_code=status.HTTP_204_NO_CONTENT)
async def permanent_delete_task(deleted_task_id: UUID, session: DatabaseSession) -> None:
    """Permanently delete a task from trash (cannot be recovered)."""
    service = TaskService(session)
    deleted_task = await service.get_deleted_task(deleted_task_id)
    if not deleted_task:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Deleted task not found")

    try:
        await service.permanent_delete(deleted_task)
        await session.commit()
    except Exception:
        await session.rollback()
        raise


@router.delete("/trash/empty", status_code=status.HTTP_200_OK)
async def empty_trash(session: DatabaseSession) -> dict[str, int]:
    """Empty trash (permanently delete all tasks in trash)."""
    service = TaskService(session)
    try:
        count = await service.empty_trash()
        await session.commit()
        return {"deleted_count": count}
    except Exception:
        await session.rollback()
        raise

