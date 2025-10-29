"""HTTP client for interacting with the backend API."""

from __future__ import annotations

import asyncio
from typing import Any, cast
from uuid import UUID

import httpx

from tui.config import TUIConfig
from tui.models import ProjectDTO, TaskDTO


class APIClient:
    """Wrapper around httpx.AsyncClient with domain-specific helpers."""

    def __init__(self, config: TUIConfig) -> None:
        self._config = config
        self._client = httpx.AsyncClient(
            base_url=config.api_base_url,
            timeout=config.request_timeout,
            trust_env=config.trust_env_proxies,
        )

    async def aclose(self) -> None:
        await self._client.aclose()

    async def _get(self, path: str, *, params: dict[str, Any] | None = None) -> Any:
        response = await self._client.get(path, params=params)
        response.raise_for_status()
        return response.json()

    async def list_projects(self) -> list[ProjectDTO]:
        projects_payload = cast(list[dict[str, Any]], await self._get("/projects/"))
        project_tasks_tuple = await asyncio.gather(
            *(self.list_tasks_raw(UUID(project["id"])) for project in projects_payload),
            return_exceptions=False,
        )
        project_tasks = list(project_tasks_tuple)

        projects: list[ProjectDTO] = []
        for payload, tasks_payload in zip(projects_payload, project_tasks, strict=False):
            tasks = [TaskDTO.from_api(task_payload) for task_payload in tasks_payload]
            projects.append(ProjectDTO.from_api(payload, tasks))
        return projects

    async def list_tasks_raw(self, project_id: UUID | None = None) -> list[dict[str, Any]]:
        params = {"project_id": str(project_id)} if project_id else None
        payload = await self._get("/tasks/", params=params)
        return cast(list[dict[str, Any]], payload)

    async def refresh_task(self, task_id: UUID) -> TaskDTO:
        payload = await self._get(f"/tasks/{task_id}/")
        return TaskDTO.from_api(payload)


__all__ = ["APIClient"]
