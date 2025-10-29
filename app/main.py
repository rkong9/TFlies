"""Application entrypoint."""

from __future__ import annotations

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.api.routes import get_api_router
from app.core.config import settings
from app.core.logging import configure_logging


def create_app() -> FastAPI:
    """Instantiate and configure the FastAPI application."""

    configure_logging()

    application = FastAPI(
        title=settings.app_name,
        version=settings.version,
        debug=settings.debug,
    )

    application.add_middleware(
        CORSMiddleware,
        allow_origins=settings.cors_allow_origins,
        allow_credentials=settings.cors_allow_credentials,
        allow_methods=settings.cors_allow_methods,
        allow_headers=settings.cors_allow_headers,
    )

    application.include_router(get_api_router(), prefix=settings.api_prefix)

    @application.get("/", include_in_schema=False)
    async def root() -> dict[str, str]:
        """Expose a simple root document for quick diagnostics."""

        return {"app": settings.app_name, "version": settings.version}

    return application


app = create_app()
