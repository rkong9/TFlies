"""Top-level stats panel with btop-inspired styling."""

from __future__ import annotations

from dataclasses import dataclass

from rich.table import Table
from textual.widgets import Static

from tui.models import ProjectDTO, TaskDTO


@dataclass(slots=True)
class AggregateStats:
    projects: int
    tasks: int
    active: int
    pending: int
    completed: int
    total_logged_ms: int
    estimated_ms: int

    @property
    def utilization(self) -> float:
        if self.estimated_ms <= 0:
            return 0.0
        return min(self.total_logged_ms / self.estimated_ms, 1.0)


def _collect_stats(projects: list[ProjectDTO]) -> AggregateStats:
    tasks: list[TaskDTO] = [task for project in projects for task in project.tasks]
    active = sum(task.status == "in_progress" for task in tasks)
    pending = sum(task.status in {"todo", "paused"} for task in tasks)
    completed = sum(task.status == "completed" for task in tasks)
    total_logged_ms = sum(task.total_logged_ms for task in tasks)
    estimated_ms = sum(task.estimated_time_ms for task in tasks)
    return AggregateStats(
        projects=len(projects),
        tasks=len(tasks),
        active=active,
        pending=pending,
        completed=completed,
        total_logged_ms=total_logged_ms,
        estimated_ms=estimated_ms,
    )


class StatsPanel(Static):
    """Displays aggregated project/task statistics with vibrant styling."""

    DEFAULT_CSS = "StatsPanel { min-height: 5; padding: 1 2; }"

    def on_mount(self) -> None:
        self.show_placeholder()

    def show_placeholder(self) -> None:
        self.update("[bold cyan]⏳ 等待数据...[/bold cyan]")

    def update_stats(self, projects: list[ProjectDTO]) -> None:
        stats = _collect_stats(projects)
        table = Table.grid(expand=True)
        table.add_column(justify="left")
        table.add_column(justify="left")
        table.add_column(justify="left")

        table.add_row(
            self._metric("Projects", stats.projects, "#38bdf8"),
            self._metric("Tasks", stats.tasks, "#c084fc"),
            self._metric("Active", stats.active, "#4ade80"),
        )
        utilization = stats.utilization * 100
        table.add_row(
            self._metric("Pending", stats.pending, "#facc15"),
            self._metric("Completed", stats.completed, "#22d3ee"),
            self._metric(
                "Utilization",
                f"{utilization:5.1f}%",
                "#f87171" if utilization >= 95 else "#fdba74" if utilization >= 70 else "#34d399",
            ),
        )

        table.add_row(
            self._metric("Logged", self._format_duration(stats.total_logged_ms), "#f472b6"),
            self._metric("Planned", self._format_duration(stats.estimated_ms), "#60a5fa"),
            self._metric(
                "Delta",
                self._format_duration(stats.total_logged_ms - stats.estimated_ms),
                "#f97316",
            ),
        )

        self.update(table)

    @staticmethod
    def _metric(label: str, value: int | str, color: str) -> str:
        return f"[dim]{label}[/dim]\n[bold {color}]{value}[/bold {color}]"

    @staticmethod
    def _format_duration(duration_ms: int) -> str:
        sign = "-" if duration_ms < 0 else ""
        ms = abs(duration_ms)
        seconds = ms / 1000
        hours, remainder = divmod(int(seconds), 3600)
        minutes, secs = divmod(remainder, 60)
        if hours:
            return f"{sign}{hours}h {minutes}m"
        if minutes:
            return f"{sign}{minutes}m {secs}s"
        return f"{sign}{secs}s"


__all__ = ["StatsPanel"]
