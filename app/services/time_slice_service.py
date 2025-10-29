"""Service layer for time slice operations."""

from __future__ import annotations

import datetime as dt
from uuid import UUID

from sqlalchemy import and_, or_, select
from sqlalchemy.ext.asyncio import AsyncSession

from app.models.time_slice import TimeSlice
from app.schemas.time_slice import TimeSliceCreate, TimeSliceUpdate


class TimeSliceService:
    """Handles creation and management of task time slices."""

    def __init__(self, session: AsyncSession) -> None:
        self._session = session

    async def create(self, data: TimeSliceCreate) -> TimeSlice:
        payload = data.model_dump()
        start = payload["start_at"]
        end = payload["end_at"]

        # 只在 end_at 存在时才验证和计算
        if end is not None:
            if start > end:
                raise ValueError("start_at must be earlier than or equal to end_at")
            payload["duration_ms"] = payload.get("duration_ms") or self._duration_ms(start, end)
            await self._ensure_no_overlap(data.task_id, start, end)
        # 如果 end_at 为 None，表示正在进行中，不需要验证重叠

        timeslice = TimeSlice(**payload)
        self._session.add(timeslice)
        await self._session.flush()
        return timeslice

    async def update(self, timeslice: TimeSlice, data: TimeSliceUpdate) -> TimeSlice:
        updates = data.model_dump(exclude_unset=True)
        start = updates.get("start_at", timeslice.start_at)
        end = updates.get("end_at", timeslice.end_at)

        # 只在 end 不为 None 时才验证和计算
        if end is not None:
            if start > end:
                raise ValueError("start_at must be earlier than or equal to end_at")
            updates.setdefault("duration_ms", self._duration_ms(start, end))
            await self._ensure_no_overlap(timeslice.task_id, start, end, exclude_id=timeslice.id)

        for field, value in updates.items():
            setattr(timeslice, field, value)
        await self._session.flush()
        return timeslice

    async def delete(self, timeslice: TimeSlice) -> None:
        await self._session.delete(timeslice)
        await self._session.flush()

    async def get(self, timeslice_id: UUID) -> TimeSlice | None:
        return await self._session.get(TimeSlice, timeslice_id)

    async def list_by_task(self, task_id: UUID) -> list[TimeSlice]:
        stmt = (
            select(TimeSlice)
            .where(TimeSlice.task_id == task_id)
            .order_by(TimeSlice.start_at.asc())
        )
        result = await self._session.execute(stmt)
        return list(result.scalars())

    async def list(self) -> list[TimeSlice]:
        stmt = select(TimeSlice).order_by(TimeSlice.start_at.desc())
        result = await self._session.execute(stmt)
        return list(result.scalars())

    async def _ensure_no_overlap(
        self,
        task_id: UUID,
        start: dt.datetime,
        end: dt.datetime,
        *,
        exclude_id: UUID | None = None,
    ) -> None:
        stmt = select(TimeSlice).where(TimeSlice.task_id == task_id)
        if exclude_id:
            stmt = stmt.where(TimeSlice.id != exclude_id)
        overlap_condition = or_(
            and_(TimeSlice.start_at <= start, TimeSlice.end_at > start),
            and_(TimeSlice.start_at < end, TimeSlice.end_at >= end),
            and_(TimeSlice.start_at >= start, TimeSlice.end_at <= end),
        )
        stmt = stmt.where(overlap_condition)
        result = await self._session.execute(stmt)
        if result.scalars().first():
            raise ValueError("time slice overlaps with an existing entry")

    @staticmethod
    def _duration_ms(start: dt.datetime, end: dt.datetime) -> int:
        delta = end - start
        return int(delta.total_seconds() * 1000)
