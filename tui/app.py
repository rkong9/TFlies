"""Textual application entrypoint for the time-box analyzer TUI."""

from __future__ import annotations

import httpx
from textual.app import App, ComposeResult
from textual.containers import Horizontal, Vertical
from textual.timer import Timer
from textual.widgets import Tree

from tui.api import APIClient
from tui.config import TUIConfig, load_config
from tui.models import ProjectDTO
from tui.widgets import (
    ActivityHeatmap,
    CommandInput,
    DetailTree,
    TaskStatusPanel,
    TaskTree,
    TimeProgressPanel,
)
from tui.widgets.task_tree import NodeData


class TimeBoxApp(App[None]):
    """Interactive terminal UI for time-box analysis."""

    CSS_PATH = "styles.tcss"
    BINDINGS = [
        ("r", "refresh", "刷新"),
        ("q", "quit", "退出"),
    ]

    def __init__(self, *, config: TUIConfig | None = None) -> None:
        super().__init__()
        self._config = config or load_config()
        self._api_client = APIClient(self._config)
        self._projects: list[ProjectDTO] = []
        self._refresh_timer: Timer | None = None
        self._client_closed = False

    def compose(self) -> ComposeResult:
        """Compose the UI layout matching the design diagram."""
        with Horizontal(id="main-container"):
            # Left column: 版块1 (TaskTree) + 版块2 (DetailTree)
            with Vertical(id="left-column"):
                yield TaskTree()
                yield DetailTree()

            # Right column: 版块3 (TimeProgress) + middle row + 版块6 (Command)
            with Vertical(id="right-column"):
                # 版块3: Time progress at top
                yield TimeProgressPanel()

                # Middle row: 版块4 (TaskStatus) + 版块5 (ActivityHeatmap)
                with Horizontal(id="middle-row"):
                    yield TaskStatusPanel()
                    yield ActivityHeatmap()

        # 版块6: Command input at bottom (docked)
        yield CommandInput()

    async def on_mount(self) -> None:
        """Initialize the app on mount."""
        await self.action_refresh()
        self._refresh_timer = self.set_interval(
            self._config.refresh_interval,
            self._handle_refresh_timer,
            pause=False,
        )

    async def on_unmount(self) -> None:
        """Clean up on unmount."""
        await self._close_api_client()

    async def _handle_refresh_timer(self) -> None:
        """Handle periodic refresh."""
        await self.action_refresh()

    async def _close_api_client(self) -> None:
        """Close the API client."""
        if not self._client_closed:
            await self._api_client.aclose()
            self._client_closed = True

    async def action_refresh(self) -> None:
        """Refresh data from the API."""
        task_tree = self.query_one(TaskTree)
        detail_tree = self.query_one(DetailTree)
        status_panel = self.query_one(TaskStatusPanel)
        time_progress = self.query_one(TimeProgressPanel)
        heatmap = self.query_one(ActivityHeatmap)

        try:
            self._projects = await self._api_client.list_projects()
        except httpx.HTTPError as exc:
            self.bell()
            status_panel.update(f"[red]数据请求失败: {exc}[/red]")
            time_progress.show_placeholder()
            heatmap.show_placeholder()
            return

        # Update task tree
        task_tree.populate(self._projects)

        # Clear detail tree
        detail_tree.clear()

        # Update status panel and other widgets based on data availability
        if self._projects:
            # Check if there are any tasks with time slices
            has_time_data = any(
                task.time_slices
                for project in self._projects
                for task in project.tasks
            )

            if has_time_data:
                time_progress._update_progress()
                heatmap._update_heatmap()
            else:
                time_progress.show_placeholder()
                heatmap.show_placeholder()

            status_panel.show_placeholder()
        else:
            status_panel.update("[dim]暂无项目数据[/dim]")
            time_progress.show_placeholder()
            heatmap.show_placeholder()

    async def on_tree_node_selected(self, event: Tree.NodeSelected[NodeData]) -> None:  # type: ignore[override]
        """Handle tree node selection."""
        data = event.node.data
        if data is None:
            return

        detail_tree = self.query_one(DetailTree)
        status_panel = self.query_one(TaskStatusPanel)

        if data.task:
            # Show task details
            detail_tree.show_task_details(data.task)
            status_panel.show_task(data.task)
        elif data.project:
            # Show project info
            detail_tree.clear()
            status_panel.update(f"[bold]项目:[/bold] {data.project.name}")


def run() -> None:
    """Convenience function for running the TUI via `python -m tui`."""
    app = TimeBoxApp()
    app.run()


if __name__ == "__main__":  # pragma: no cover
    run()
