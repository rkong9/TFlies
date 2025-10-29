"""Detailed information panel for selected project or task."""

from __future__ import annotations

import datetime as dt

from rich.table import Table
from textual.widgets import Static

from tui.models import ProjectDTO, TaskDTO, TimeSliceDTO


def _format_datetime(value: dt.datetime | None) -> str:
    if value is None:
        return "—"
    return value.astimezone(dt.UTC).strftime("%Y-%m-%d %H:%M:%S %Z")


def _format_duration_ms(value: int) -> str:
    seconds = value / 1000
    hours, remainder = divmod(int(seconds), 3600)
    minutes, secs = divmod(remainder, 60)
    frac = seconds - int(seconds)
    if hours:
        return f"{hours}h {minutes}m {secs + frac:.1f}s"
    if minutes:
        return f"{minutes}m {secs + frac:.1f}s"
    return f"{seconds:.1f}s"


def _build_time_slice_table(time_slices: list[TimeSliceDTO]) -> Table:
    table = Table(title="Time Slices", show_header=True, expand=True)
    table.add_column("Start", justify="left")
    table.add_column("End", justify="left")
    table.add_column("Duration", justify="right")
    table.add_column("Score", justify="center")
    if not time_slices:
        table.add_row("—", "—", "—", "—")
    else:
        for slice_ in sorted(
            time_slices,
            key=lambda ts: ts.start_at or dt.datetime.min.replace(tzinfo=dt.UTC),
        ):
            table.add_row(
                _format_datetime(slice_.start_at),
                _format_datetime(slice_.end_at),
                _format_duration_ms(slice_.duration_ms),
                str(slice_.efficiency_score),
            )
    return table


def _build_task_table(task: TaskDTO) -> Table:
    table = Table.grid(padding=(0, 1))
    table.add_row("Title", f"[b]{task.title}[/b]")
    table.add_row("Status", task.status)
    table.add_row("Priority", str(task.priority))
    table.add_row("Due", _format_datetime(task.due_at))
    table.add_row("Estimated", _format_duration_ms(task.estimated_time_ms))
    table.add_row("Logged", _format_duration_ms(task.total_logged_ms))
    if task.description:
        table.add_row("Description", task.description)
    return table


def _build_project_table(project: ProjectDTO) -> Table:
    table = Table.grid(padding=(0, 1))
    table.add_row("Name", f"[b]{project.name}[/b]")
    table.add_row("Planned", _format_duration_ms(project.planned_time_ms))
    table.add_row("Logged", _format_duration_ms(project.total_logged_ms))
    table.add_row("Start", _format_datetime(project.start_at))
    table.add_row("End", _format_datetime(project.end_at))
    if project.description:
        table.add_row("Description", project.description)
    return table


class DetailPanel(Static):
    """Displays contextual information for the focused node."""

    DEFAULT_CSS = "DetailPanel { }"

    def on_mount(self) -> None:
        self.show_message("欢迎使用：请选择左侧的项目或任务。")

    def show_message(self, message: str) -> None:
        self.update(message)

    def show_project(self, project: ProjectDTO) -> None:
        project_table = _build_project_table(project)
        self.update(project_table)

    def show_task(self, task: TaskDTO) -> None:
        task_table = _build_task_table(task)
        time_slice_table = _build_time_slice_table(task.time_slices)
        grid = Table.grid(expand=True)
        grid.add_row(task_table)
        grid.add_row(time_slice_table)
        self.update(grid)
