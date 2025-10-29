"""Time slice API endpoints."""

from __future__ import annotations

from typing import Annotated
from uuid import UUID

from fastapi import APIRouter, HTTPException, Query, status

from app.api.dependencies import DatabaseSession
from app.schemas.time_slice import TimeSliceCreate, TimeSliceRead, TimeSliceUpdate
from app.services.time_slice_service import TimeSliceService

router = APIRouter(prefix="/time-slices", tags=["TimeSlices"])


@router.post("/", response_model=TimeSliceRead, status_code=status.HTTP_201_CREATED)
async def create_time_slice(payload: TimeSliceCreate, session: DatabaseSession) -> TimeSliceRead:
    service = TimeSliceService(session)
    try:
        timeslice = await service.create(payload)
        await session.commit()
        await session.refresh(timeslice)
        return TimeSliceRead.model_validate(timeslice)
    except Exception as exc:
        await session.rollback()
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail=str(exc)) from exc


@router.get("/", response_model=list[TimeSliceRead])
async def list_time_slices(
    session: DatabaseSession,
    task_id: Annotated[UUID | None, Query()] = None,
) -> list[TimeSliceRead]:
    service = TimeSliceService(session)
    slices = await (service.list_by_task(task_id) if task_id else service.list())
    return [TimeSliceRead.model_validate(item) for item in slices]


@router.get("/{timeslice_id}", response_model=TimeSliceRead)
async def get_time_slice(timeslice_id: UUID, session: DatabaseSession) -> TimeSliceRead:
    service = TimeSliceService(session)
    timeslice = await service.get(timeslice_id)
    if not timeslice:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Time slice not found")
    return TimeSliceRead.model_validate(timeslice)


@router.patch("/{timeslice_id}", response_model=TimeSliceRead)
async def update_time_slice(
    timeslice_id: UUID,
    payload: TimeSliceUpdate,
    session: DatabaseSession,
) -> TimeSliceRead:
    service = TimeSliceService(session)
    timeslice = await service.get(timeslice_id)
    if not timeslice:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Time slice not found")
    try:
        await service.update(timeslice, payload)
        await session.commit()
        await session.refresh(timeslice)
        return TimeSliceRead.model_validate(timeslice)
    except Exception as exc:
        await session.rollback()
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail=str(exc)) from exc


@router.delete("/{timeslice_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_time_slice(timeslice_id: UUID, session: DatabaseSession) -> None:
    service = TimeSliceService(session)
    timeslice = await service.get(timeslice_id)
    if not timeslice:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Time slice not found")
    try:
        await service.delete(timeslice)
        await session.commit()
    except Exception:
        await session.rollback()
        raise
