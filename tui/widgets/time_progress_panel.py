"""Time progress panel with efficiency heatmap (版块3)."""

from __future__ import annotations

import datetime as dt

from rich.console import Group
from rich.text import Text
from textual.widgets import Static


class TimeProgressPanel(Static):
    """Displays year/month/week/day progress bars with efficiency heatmap."""

    DEFAULT_CSS = """
    TimeProgressPanel {
        height: auto;
    }
    """

    def on_mount(self) -> None:
        self.show_placeholder()

    def show_placeholder(self) -> None:
        """Show placeholder when no task data available."""
        self.update("[dim]等待任务数据...[/dim]")

    def _update_progress(self) -> None:
        """Update all time progress bars."""
        now = dt.datetime.now(dt.UTC)

        # Calculate progress for different time periods
        year_progress = self._calc_year_progress(now)
        month_progress = self._calc_month_progress(now)
        week_progress = self._calc_week_progress(now)
        day_progress = self._calc_day_progress(now)

        # Create progress bars
        lines = [
            Text("⏱️  时间进度", style="bold"),
            Text(""),
            self._render_progress_bar("年", year_progress, 35),
            self._render_progress_bar("月", month_progress, 35),
            self._render_progress_bar("周", week_progress, 35),
            self._render_progress_bar("日", day_progress, 35),
        ]

        self.update(Group(*lines))

    @staticmethod
    def _calc_year_progress(now: dt.datetime) -> float:
        """Calculate progress through the current year."""
        start_of_year = now.replace(month=1, day=1, hour=0, minute=0, second=0, microsecond=0)
        is_leap = now.year % 4 == 0 and (now.year % 100 != 0 or now.year % 400 == 0)
        days_in_year = 366 if is_leap else 365
        elapsed = (now - start_of_year).total_seconds()
        total = days_in_year * 24 * 3600
        return elapsed / total

    @staticmethod
    def _calc_month_progress(now: dt.datetime) -> float:
        """Calculate progress through the current month."""
        start_of_month = now.replace(day=1, hour=0, minute=0, second=0, microsecond=0)
        # Calculate days in current month
        if now.month == 12:
            next_month = now.replace(year=now.year + 1, month=1, day=1)
        else:
            next_month = now.replace(month=now.month + 1, day=1)
        days_in_month = (next_month - start_of_month).days
        elapsed = (now - start_of_month).total_seconds()
        total = days_in_month * 24 * 3600
        return elapsed / total

    @staticmethod
    def _calc_week_progress(now: dt.datetime) -> float:
        """Calculate progress through the current week."""
        start_of_week = now - dt.timedelta(days=now.weekday())
        start_of_week = start_of_week.replace(hour=0, minute=0, second=0, microsecond=0)
        elapsed = (now - start_of_week).total_seconds()
        total = 7 * 24 * 3600
        return elapsed / total

    @staticmethod
    def _calc_day_progress(now: dt.datetime) -> float:
        """Calculate progress through the current day."""
        start_of_day = now.replace(hour=0, minute=0, second=0, microsecond=0)
        elapsed = (now - start_of_day).total_seconds()
        total = 24 * 3600
        return elapsed / total

    @staticmethod
    def _render_progress_bar(label: str, progress: float, width: int) -> Text:
        """Render a single progress bar with efficiency heatmap."""
        # TODO: Add efficiency data when available
        filled_width = int(progress * width)
        empty_width = width - filled_width

        # Color based on "efficiency" (placeholder: use progress as mock efficiency)
        if progress < 0.3:
            color = "green"
        elif progress < 0.6:
            color = "yellow"
        else:
            color = "red"

        bar = Text()
        bar.append(f"{label} ", style="dim")
        bar.append("█" * filled_width, style=f"bold {color}")
        bar.append("░" * empty_width, style="dim")
        bar.append(f" {progress * 100:5.1f}%", style="dim")

        return bar


__all__ = ["TimeProgressPanel"]
