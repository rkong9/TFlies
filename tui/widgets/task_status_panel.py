"""Task status and description panel (版块4)."""

from __future__ import annotations

from rich.console import Group
from rich.panel import Panel
from rich.text import Text
from textual.widgets import Static

from tui.models import TaskDTO


class TaskStatusPanel(Static):
    """Displays task status bar and description."""

    DEFAULT_CSS = """
    TaskStatusPanel {
        height: auto;
    }
    """

    def __init__(self) -> None:
        super().__init__()
        self._current_task: TaskDTO | None = None

    def on_mount(self) -> None:
        self.show_placeholder()

    def show_placeholder(self) -> None:
        """Show placeholder when no task is selected."""
        self.update("[dim]← 请从左侧选择任务[/dim]")

    def show_task(self, task: TaskDTO) -> None:
        """Display task status and description."""
        self._current_task = task

        # Status bar
        status_icons = {
            "todo": "⬜ 待办",
            "in_progress": "⏳ 进行中",
            "paused": "⏸️  暂停",
            "completed": "✅ 已完成",
            "cancelled": "🚫 已取消",
        }
        status_text = status_icons.get(task.status, task.status)

        # Calculate efficiency if possible
        efficiency_text = ""
        if task.estimated_time_ms > 0:
            usage_pct = (task.total_logged_ms / task.estimated_time_ms) * 100
            if usage_pct < 80:
                efficiency_text = f" | 时间利用率: [green]{usage_pct:.1f}%[/green]"
            elif usage_pct < 100:
                efficiency_text = f" | 时间利用率: [yellow]{usage_pct:.1f}%[/yellow]"
            else:
                efficiency_text = f" | 时间利用率: [red]{usage_pct:.1f}%[/red] ⚠️ 超时"

        status_bar = Text()
        status_bar.append("状态: ", style="dim")
        status_bar.append(status_text, style="bold")
        status_bar.append(efficiency_text)

        # Priority
        priority_text = Text()
        priority_text.append("优先级: ", style="dim")
        if task.priority <= 2:
            priority_color = "red"
        elif task.priority <= 3:
            priority_color = "yellow"
        else:
            priority_color = "green"
        priority_text.append(f"P{task.priority}", style=f"bold {priority_color}")

        # Description
        description = task.description or "[dim italic]无描述[/dim italic]"

        # Combine everything
        content = Group(
            status_bar,
            priority_text,
            Text(""),
            Panel(description, title="任务描述", border_style="dim"),
        )

        self.update(content)


__all__ = ["TaskStatusPanel"]
