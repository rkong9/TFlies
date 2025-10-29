"""Tree widget visualising projects and their nested tasks."""

from __future__ import annotations

import datetime as dt
from collections import defaultdict
from collections.abc import Iterable
from dataclasses import dataclass

from textual.widgets import Tree
from textual.widgets._tree import TreeNode

from tui.models import ProjectDTO, TaskDTO

_STATUS_MARKERS = {
    "todo": "⬜",
    "in_progress": "⏳",
    "paused": "⏸️",
    "completed": "✅",
    "cancelled": "🚫",
}


@dataclass(slots=True)
class NodeData:
    project: ProjectDTO | None = None
    task: TaskDTO | None = None


class TaskTree(Tree[NodeData]):
    """Tree widget displaying project and task hierarchy."""

    def __init__(self) -> None:
        super().__init__("📋 任务", data=NodeData(), id="task-tree")
        self.show_root = True
        self.show_guides = True

    def populate(self, projects: Iterable[ProjectDTO]) -> None:
        """Populate the tree from the provided projects."""

        self.root.remove_children()
        for project in projects:
            project_node = self.root.add(
                f"📁 [b]{project.name}[/b]",
                data=NodeData(project=project),
            )
            if project.tasks:
                project_node.allow_expand = True
                self._populate_tasks(project_node, project.tasks)
                project_node.expand()
            else:
                project_node.allow_expand = False
        self.root.expand()

    def _populate_tasks(self, parent_node: TreeNode, tasks: list[TaskDTO]) -> None:
        tasks_by_parent: dict[str | None, list[TaskDTO]] = defaultdict(list)
        for task in tasks:
            tasks_by_parent[task.parent_id.hex if task.parent_id else None].append(task)

        def add_children(node: TreeNode, parent_id: str | None) -> None:
            ordered = sorted(
                tasks_by_parent.get(parent_id, []),
                key=lambda t: (t.priority, t.created_at or dt.datetime.min.replace(tzinfo=dt.UTC)),
            )
            for task in ordered:
                label = self._format_task_label(task)
                child_node = node.add(label, data=NodeData(task=task))
                child_id = task.id.hex
                has_children = child_id in tasks_by_parent
                if has_children:
                    child_node.allow_expand = True
                    add_children(child_node, child_id)
                else:
                    child_node.allow_expand = False

        add_children(parent_node, None)

    @staticmethod
    def _format_task_label(task: TaskDTO) -> str:
        marker = _STATUS_MARKERS.get(task.status, "•")
        total_logged = task.total_logged_ms / 1000
        estimated = task.estimated_time_ms / 1000 if task.estimated_time_ms else 0
        eta = f" {total_logged:.1f}s" if total_logged else ""
        if estimated:
            eta = f" {total_logged:.1f}s / {estimated:.1f}s"
        return f"{marker} [b]{task.title}[/b] [dim]{task.status}{eta}[/dim]"


__all__ = ["TaskTree", "NodeData"]
