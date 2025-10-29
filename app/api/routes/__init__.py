"""API routing helpers."""

from fastapi import APIRouter

from . import health, projects, tasks, time_slices


def get_api_router() -> APIRouter:
    """Return the root API router with all sub-routers registered."""

    router = APIRouter()
    router.include_router(health.router)
    router.include_router(projects.router)
    router.include_router(tasks.router)
    router.include_router(time_slices.router)
    return router


__all__ = ["get_api_router"]
