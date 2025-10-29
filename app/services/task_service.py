"""Service layer for task operations."""

from __future__ import annotations

import datetime as dt
from uuid import UUID

from sqlalchemy import func, select
from sqlalchemy.ext.asyncio import AsyncSession

from app.core.config import get_settings
from app.models.deleted_task import DeletedTask
from app.models.project import Project
from app.models.task import ExecutionState, Task, TaskStatus
from app.models.time_slice import TimeSlice
from app.schemas.task import TaskCreate, TaskUpdate


class TaskService:
    """Encapsulates operations related to task management."""

    def __init__(self, session: AsyncSession) -> None:
        self._session = session

    async def create(self, data: TaskCreate) -> Task:
        payload = data.model_dump()
        task = Task(**payload)
        self._session.add(task)
        await self._session.flush()
        if task.parent_id:
            parent = await self.get(task.parent_id)
            if parent:
                await self._ensure_parent_state(parent)
        return task

    async def get(self, task_id: UUID) -> Task | None:
        return await self._session.get(Task, task_id)

    async def list_by_project(self, project_id: UUID) -> list[Task]:
        stmt = (
            select(Task)
            .where(Task.project_id == project_id)
            .order_by(Task.position.asc(), Task.created_at.asc())
        )
        result = await self._session.execute(stmt)
        return list(result.scalars())

    async def list(self) -> list[Task]:
        stmt = select(Task).order_by(Task.position.asc(), Task.created_at.asc())
        result = await self._session.execute(stmt)
        return list(result.scalars())

    async def update(self, task: Task, data: TaskUpdate) -> Task:
        # 检查任务是否因DDL过期而被锁定
        if await self._is_task_locked(task):
            from fastapi import HTTPException, status
            raise HTTPException(
                status_code=status.HTTP_403_FORBIDDEN,
                detail="任务已因DDL过期而锁定，无法修改"
            )

        updates = data.model_dump(exclude_unset=True)

        # 检查执行状态互斥逻辑
        if "execution_state" in updates and updates["execution_state"] == ExecutionState.WORKING:
            # 查询是否有其他任务正在工作
            stmt = select(Task).where(
                Task.id != task.id,
                Task.execution_state == ExecutionState.WORKING
            )
            result = await self._session.execute(stmt)
            working_task = result.scalar_one_or_none()

            if working_task:
                from fastapi import HTTPException, status
                raise HTTPException(
                    status_code=status.HTTP_409_CONFLICT,
                    detail=f"任务「{working_task.title}」正在工作中，同一时间只能有一个任务处于工作状态"
                )

        # 检查父任务循环引用
        if "parent_id" in updates and updates["parent_id"] is not None:
            new_parent_id = updates["parent_id"]
            # 检查是否会造成循环引用
            if await self._would_create_cycle(task.id, new_parent_id):
                from fastapi import HTTPException, status
                raise HTTPException(
                    status_code=status.HTTP_400_BAD_REQUEST,
                    detail="无法设置该父任务：会造成循环引用"
                )

        for field, value in updates.items():
            setattr(task, field, value)
        await self._session.flush()
        if task.parent_id:
            parent = await self.get(task.parent_id)
            if parent:
                await self._ensure_parent_state(parent)
        await self._session.refresh(task, attribute_names=["children"])
        if task.children:
            await self._cascade_status_to_children(task, task.status)
        return task

    async def delete(self, task: Task) -> None:
        """Delete a task and save it to trash for potential recovery.

        This performs a soft delete by:
        1. Creating a complete snapshot of the task tree (task + children + time slices)
        2. Saving the snapshot to the deleted_task table
        3. Performing the actual database deletion (CASCADE handles children/slices)
        """
        # 检查任务是否因DDL过期而被锁定
        if await self._is_task_locked(task):
            from fastapi import HTTPException, status
            raise HTTPException(
                status_code=status.HTTP_403_FORBIDDEN,
                detail="任务已因DDL过期而锁定，无法删除"
            )

        parent_id = task.parent_id

        # Create snapshot before deletion
        await self._save_to_trash(task)

        # Perform actual deletion
        await self._session.delete(task)
        await self._session.flush()

        # Update parent state if exists
        if parent_id:
            parent = await self.get(parent_id)
            if parent:
                await self._ensure_parent_state(parent)

    async def _save_to_trash(self, task: Task) -> None:
        """Save complete task tree snapshot to trash."""
        settings = get_settings()

        # Build complete snapshot
        snapshot = await self._build_task_snapshot(task)

        # Calculate expiry date
        now = dt.datetime.now(dt.timezone.utc)
        expiry_at = now + dt.timedelta(days=settings.trash_retention_days)

        # Create deleted task record
        deleted_task = DeletedTask(
            task_id=task.id,
            deleted_at=now,
            expiry_at=expiry_at,
            snapshot=snapshot,
            deletion_metadata={
                "project_id": str(task.project_id),
                "title": task.title,
                "total_descendants": self._count_descendants(snapshot),
            }
        )

        self._session.add(deleted_task)
        await self._session.flush()

    async def _build_task_snapshot(self, task: Task) -> dict:
        """Recursively build complete task tree snapshot including children and time slices."""
        # Ensure relationships are loaded
        await self._session.refresh(task, attribute_names=["children", "time_slices"])

        # Serialize main task
        task_data = {
            "id": str(task.id),
            "title": task.title,
            "description": task.description,
            "priority": task.priority,
            "due_at": task.due_at.isoformat() if task.due_at else None,
            "estimated_time_ms": task.estimated_time_ms,
            "status": task.status.value,
            "execution_state": task.execution_state.value,
            "project_id": str(task.project_id),
            "parent_id": str(task.parent_id) if task.parent_id else None,
            "created_at": task.created_at.isoformat(),
            "updated_at": task.updated_at.isoformat(),
        }

        # Serialize time slices
        time_slices = [
            {
                "id": str(ts.id),
                "start_at": ts.start_at.isoformat(),
                "end_at": ts.end_at.isoformat(),
                "duration_ms": ts.duration_ms,
                "efficiency_score": ts.efficiency_score,
                "note": ts.note,
                "created_at": ts.created_at.isoformat(),
                "updated_at": ts.updated_at.isoformat(),
            }
            for ts in task.time_slices
        ]

        # Recursively serialize children
        children = []
        for child in task.children:
            child_snapshot = await self._build_task_snapshot(child)
            children.append(child_snapshot)

        return {
            "task": task_data,
            "time_slices": time_slices,
            "children": children,
        }

    def _count_descendants(self, snapshot: dict) -> int:
        """Count total number of tasks in snapshot tree."""
        count = 1  # Current task
        for child in snapshot.get("children", []):
            count += self._count_descendants(child)
        return count

    async def _cascade_status_to_children(self, task: Task, status: TaskStatus) -> None:
        await self._session.refresh(task, attribute_names=["children"])
        for child in task.children:
            child.status = status
        await self._session.flush()

    async def _is_task_locked(self, task: Task) -> bool:
        """检查任务是否因DDL过期而被锁定.

        任务被锁定的条件：
        1. 任务或其父任务启用了DDL模式 (is_ddl=True)
        2. 任务或其父任务设置了截止时间 (due_at)
        3. 当前时间超过了截止时间

        子任务会继承父任务的DDL和截止时间设置。
        """
        # 获取有效的DDL设置和截止时间（考虑父任务继承）
        effective_is_ddl = await self._get_effective_is_ddl(task)
        effective_due_at = await self._get_effective_due_at(task)

        # 如果没有启用DDL或没有截止时间，则不锁定
        if not effective_is_ddl or not effective_due_at:
            return False

        # 检查是否已过期
        now = dt.datetime.now(dt.timezone.utc)
        return now > effective_due_at

    async def _get_effective_is_ddl(self, task: Task) -> bool:
        """递归获取有效的DDL设置（子任务继承父任务）"""
        if task.is_ddl:
            return True

        if task.parent_id:
            parent = await self.get(task.parent_id)
            if parent:
                return await self._get_effective_is_ddl(parent)

        return False

    async def _get_effective_due_at(self, task: Task) -> dt.datetime | None:
        """递归获取有效的截止时间（子任务继承父任务）"""
        if task.due_at:
            return task.due_at

        if task.parent_id:
            parent = await self.get(task.parent_id)
            if parent:
                return await self._get_effective_due_at(parent)

        return None

    async def _would_create_cycle(self, task_id: UUID, new_parent_id: UUID) -> bool:
        """检查设置new_parent_id是否会造成循环引用"""
        # 如果新父任务就是自己，直接返回True
        if task_id == new_parent_id:
            return True

        # 向上遍历新父任务的祖先链，看是否会遇到task_id
        current_id = new_parent_id
        visited = set()

        while current_id is not None:
            # 防止无限循环（虽然理论上不应该发生）
            if current_id in visited:
                return True
            visited.add(current_id)

            # 如果遇到了task_id，说明会形成循环
            if current_id == task_id:
                return True

            # 获取当前任务的父任务
            current_task = await self.get(current_id)
            if not current_task:
                break
            current_id = current_task.parent_id

        return False

    async def _ensure_parent_state(self, task: Task) -> None:
        await self._session.refresh(task, attribute_names=["children"])
        if not task.children:
            return
        if all(child.status == TaskStatus.COMPLETED for child in task.children):
            task.status = TaskStatus.COMPLETED
        elif any(child.status == TaskStatus.IN_PROGRESS for child in task.children):
            task.status = TaskStatus.IN_PROGRESS
        else:
            task.status = TaskStatus.TODO
        await self._session.flush()
        if task.parent_id:
            parent = await self.get(task.parent_id)
            if parent:
                await self._ensure_parent_state(parent)

    # Trash/Recycle bin operations

    async def list_trash(self) -> list[DeletedTask]:
        """List all tasks in trash (not yet expired)."""
        now = dt.datetime.now(dt.timezone.utc)
        stmt = (
            select(DeletedTask)
            .where(DeletedTask.expiry_at > now)
            .order_by(DeletedTask.deleted_at.desc())
        )
        result = await self._session.execute(stmt)
        return list(result.scalars())

    async def get_deleted_task(self, deleted_task_id: UUID) -> DeletedTask | None:
        """Get a specific deleted task by its ID."""
        return await self._session.get(DeletedTask, deleted_task_id)

    async def restore_from_trash(self, deleted_task: DeletedTask) -> Task:
        """Restore a task from trash.

        This recreates the entire task tree (task + children + time slices)
        from the saved snapshot.
        """
        snapshot = deleted_task.snapshot

        # Verify project still exists
        project_id = UUID(snapshot["task"]["project_id"])
        project = await self._session.get(Project, project_id)
        if not project:
            from fastapi import HTTPException, status
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail=f"项目不存在，无法恢复任务"
            )

        # Restore task tree
        restored_task = await self._restore_task_tree(snapshot, project_id)

        # Delete from trash
        await self._session.delete(deleted_task)
        await self._session.flush()

        return restored_task

    async def _restore_task_tree(
        self,
        snapshot: dict,
        project_id: UUID,
        parent_id: UUID | None = None
    ) -> Task:
        """Recursively restore task tree from snapshot."""
        task_data = snapshot["task"]

        # Create task with original data (but new IDs for safety)
        task = Task(
            title=task_data["title"],
            description=task_data["description"],
            priority=task_data["priority"],
            due_at=dt.datetime.fromisoformat(task_data["due_at"]) if task_data["due_at"] else None,
            estimated_time_ms=task_data["estimated_time_ms"],
            status=TaskStatus(task_data["status"]),
            execution_state=ExecutionState(task_data["execution_state"]),
            project_id=project_id,
            parent_id=parent_id,
        )

        self._session.add(task)
        await self._session.flush()  # Generate task ID

        # Restore time slices
        for ts_data in snapshot["time_slices"]:
            time_slice = TimeSlice(
                task_id=task.id,
                start_at=dt.datetime.fromisoformat(ts_data["start_at"]),
                end_at=dt.datetime.fromisoformat(ts_data["end_at"]),
                duration_ms=ts_data["duration_ms"],
                efficiency_score=ts_data["efficiency_score"],
                note=ts_data["note"],
            )
            self._session.add(time_slice)

        # Recursively restore children
        for child_snapshot in snapshot["children"]:
            await self._restore_task_tree(child_snapshot, project_id, task.id)

        await self._session.flush()
        return task

    async def permanent_delete(self, deleted_task: DeletedTask) -> None:
        """Permanently delete a task from trash (cannot be recovered)."""
        await self._session.delete(deleted_task)
        await self._session.flush()

    async def empty_trash(self) -> int:
        """Permanently delete all tasks in trash. Returns number of tasks deleted."""
        stmt = select(DeletedTask)
        result = await self._session.execute(stmt)
        deleted_tasks = list(result.scalars())

        count = len(deleted_tasks)
        for dt_task in deleted_tasks:
            await self._session.delete(dt_task)

        await self._session.flush()
        return count

    async def cleanup_expired_trash(self) -> int:
        """Delete expired trash items. Returns number of items deleted.

        This should be called periodically (e.g., daily) to remove old trash items.
        """
        now = dt.datetime.now(dt.timezone.utc)
        stmt = select(DeletedTask).where(DeletedTask.expiry_at <= now)
        result = await self._session.execute(stmt)
        expired_tasks = list(result.scalars())

        count = len(expired_tasks)
        for dt_task in expired_tasks:
            await self._session.delete(dt_task)

        await self._session.flush()
        return count

    # Task position/ordering methods

    async def update_position(self, task: Task, new_position: float) -> Task:
        """Update task position for drag-and-drop sorting.

        Args:
            task: The task to update
            new_position: New position value (float)

        Returns:
            Updated task
        """
        # 检查任务是否因DDL过期而被锁定
        if await self._is_task_locked(task):
            from fastapi import HTTPException, status
            raise HTTPException(
                status_code=status.HTTP_403_FORBIDDEN,
                detail="任务已因DDL过期而锁定，无法移动"
            )

        task.position = new_position
        await self._session.flush()
        return task

    async def get_siblings(self, task: Task) -> list[Task]:
        """Get sibling tasks (tasks with the same parent_id) ordered by position.

        Args:
            task: The task to find siblings for

        Returns:
            List of sibling tasks (including the task itself)
        """
        stmt = (
            select(Task)
            .where(
                Task.project_id == task.project_id,
                Task.parent_id == task.parent_id
            )
            .order_by(Task.position.asc(), Task.created_at.asc())
        )
        result = await self._session.execute(stmt)
        return list(result.scalars())

    async def unlock_task(self, task: Task) -> Task:
        """Unlock a DDL-locked task by disabling its DDL mode.

        This allows the task to be modified even if the deadline has passed.
        Only affects the current task, not its parent or children.

        Args:
            task: The task to unlock

        Returns:
            Updated task with is_ddl set to False
        """
        task.is_ddl = False
        await self._session.flush()
        return task

