"""GitHub-style activity heatmap (版块5)."""

from __future__ import annotations

import datetime as dt

from rich.text import Text
from textual.widgets import Static


class ActivityHeatmap(Static):
    """Displays a GitHub-style activity heatmap showing daily efficiency."""

    DEFAULT_CSS = """
    ActivityHeatmap {
        height: auto;
    }
    """

    def on_mount(self) -> None:
        self.show_placeholder()

    def show_placeholder(self) -> None:
        """Show placeholder when no task data available."""
        self.update("[dim]等待任务数据...[/dim]")

    def _update_heatmap(self) -> None:
        """Update the heatmap display."""
        now = dt.datetime.now(dt.UTC)

        # Create heatmap for the current day (24 hours)
        lines = [
            Text("📈 今日活跃度", style="bold"),
            Text(""),
            self._render_daily_heatmap(now),
        ]

        self.update("\n".join(str(line) for line in lines))

    def _render_daily_heatmap(self, now: dt.datetime) -> Text:
        """Render a 24-hour heatmap for today."""
        # TODO: Get actual efficiency data from API
        # For now, create a placeholder with current hour highlighted

        heatmap = Text()
        current_hour = now.hour

        # Time labels
        heatmap.append("时刻: ", style="dim")

        # Render 24 blocks representing each hour
        for hour in range(24):
            if hour == current_hour:
                # Current hour - highlight
                heatmap.append("█", style="bold yellow")
            elif hour < current_hour:
                # Past hours - use placeholder efficiency colors
                # TODO: Replace with actual efficiency data
                intensity = (hour % 4) + 1  # Mock intensity 1-4
                color = self._get_intensity_color(intensity)
                heatmap.append("█", style=color)
            else:
                # Future hours - dim
                heatmap.append("░", style="dim")

        heatmap.append(f"  (当前: {current_hour:02d}:00)", style="dim")

        # Add hour scale
        scale = Text()
        scale.append("      ", style="dim")
        for hour in [0, 6, 12, 18, 24]:
            scale.append(f"{hour:02d}".ljust(6), style="dim")

        result = Text()
        result.append(heatmap)
        result.append("\n")
        result.append(scale)

        return result

    @staticmethod
    def _get_intensity_color(intensity: int) -> str:
        """Map intensity (1-5) to color."""
        if intensity >= 5:
            return "bold green"
        elif intensity >= 4:
            return "green"
        elif intensity >= 3:
            return "yellow"
        elif intensity >= 2:
            return "red"
        else:
            return "dim red"


__all__ = ["ActivityHeatmap"]
