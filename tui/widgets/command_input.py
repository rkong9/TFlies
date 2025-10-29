"""Command input widget (版块6)."""

from __future__ import annotations

from textual import events
from textual.widgets import Input


class CommandInput(Input):
    """Command-line style input for task operations."""

    DEFAULT_CSS = """
    CommandInput {
        height: 1;
        border: none;
    }
    """

    def __init__(self) -> None:
        super().__init__(placeholder="输入命令... (例如: start <task_id>, stop, help)")

    async def on_key(self, event: events.Key) -> None:
        """Handle key events."""
        if event.key == "enter":
            command = self.value.strip()
            if command:
                await self._execute_command(command)
                self.value = ""

    async def _execute_command(self, command: str) -> None:
        """Execute a command."""
        # Parse command
        parts = command.split()
        if not parts:
            return

        cmd = parts[0].lower()

        # TODO: Implement actual command handlers
        # For now, just post a message to the app
        if cmd == "help":
            self.app.bell()
            # TODO: Show help modal
        elif cmd == "start":
            # TODO: Start task timer
            pass
        elif cmd == "stop":
            # TODO: Stop current task timer
            pass
        elif cmd == "refresh" or cmd == "r":
            # Trigger refresh action
            await self.app.action_refresh()  # type: ignore[attr-defined]
        elif cmd == "quit" or cmd == "q":
            self.app.exit()
        else:
            self.app.bell()
            # TODO: Show error message


__all__ = ["CommandInput"]
