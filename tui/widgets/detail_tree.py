"""Detail tree showing time slices and subtasks (版块2)."""

from __future__ import annotations

import datetime as dt
from dataclasses import dataclass

from textual.widgets import Tree

from tui.models import TaskDTO, TimeSliceDTO


@dataclass(slots=True)
class DetailNodeData:
    """Data attached to detail tree nodes."""

    task: TaskDTO | None = None
    time_slice: TimeSliceDTO | None = None
    project: None = None  # Always None for DetailTree nodes


class DetailTree(Tree[DetailNodeData]):
    """Tree showing detailed breakdown of selected task."""

    DEFAULT_CSS = """
    DetailTree {
        height: 1fr;
    }
    """

    def __init__(self) -> None:
        super().__init__("📊 详情", data=DetailNodeData(), id="detail-tree")
        self.show_root = True
        self.show_guides = True

    def show_task_details(self, task: TaskDTO) -> None:
        """Display time slices and subtasks for a task."""
        self.root.remove_children()

        # Add time slices section
        if task.time_slices:
            slices_node = self.root.add(
                f"⏱️  时间片 ({len(task.time_slices)})",
                data=DetailNodeData(),
            )
            slices_node.allow_expand = True

            for time_slice in sorted(
                task.time_slices,
                key=lambda ts: ts.start_at or dt.datetime.min.replace(tzinfo=dt.UTC),
                reverse=True,
            ):
                label = self._format_time_slice_label(time_slice)
                slices_node.add(label, data=DetailNodeData(time_slice=time_slice))

            slices_node.expand()

        # Add subtasks section (if any children exist)
        # Note: In current data model, children are in task.children
        # TODO: Fetch subtasks when API supports it

        self.root.expand()

    def clear(self) -> DetailTree:
        """Clear the detail tree."""
        self.root.remove_children()
        placeholder = self.root.add("[dim]选择任务以查看详情[/dim]", data=DetailNodeData())
        placeholder.allow_expand = False
        return self

    @staticmethod
    def _format_time_slice_label(time_slice: TimeSliceDTO) -> str:
        """Format a time slice for display."""
        duration_ms = time_slice.duration_ms
        duration_sec = duration_ms / 1000
        duration_str = f"{duration_sec:.1f}s"

        if duration_sec >= 3600:
            hours = int(duration_sec // 3600)
            minutes = int((duration_sec % 3600) // 60)
            duration_str = f"{hours}h {minutes}m"
        elif duration_sec >= 60:
            minutes = int(duration_sec // 60)
            seconds = int(duration_sec % 60)
            duration_str = f"{minutes}m {seconds}s"

        start_time = ""
        if time_slice.start_at:
            start_time = time_slice.start_at.astimezone().strftime("%H:%M:%S")

        efficiency = "⭐" * time_slice.efficiency_score
        note_preview = ""
        if time_slice.note:
            note_preview = f" - {time_slice.note[:30]}"

        return f"{start_time} [{duration_str}] {efficiency}{note_preview}"


__all__ = ["DetailTree", "DetailNodeData"]
