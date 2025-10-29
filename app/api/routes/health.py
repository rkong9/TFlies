"""Basic liveness and readiness endpoints."""

from fastapi import APIRouter

router = APIRouter(tags=["Health"], prefix="/health")


@router.get("/live", summary="Liveness probe")
async def live() -> dict[str, str]:
    """Return service availability for liveness checks."""

    return {"status": "ok"}


@router.get("/ready", summary="Readiness probe")
async def ready() -> dict[str, str]:
    """Return readiness status for orchestrators."""

    return {"status": "ready"}
