"""add_position_field_to_task

Revision ID: b530655efe65
Revises: 0236a1bfc28b
Create Date: 2025-11-10 19:18:29.730292

"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision: str = 'b530655efe65'
down_revision: Union[str, Sequence[str], None] = '0236a1bfc28b'
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """Add position field to task table for drag-and-drop sorting."""

    # Add position column
    op.add_column('task', sa.Column('position', sa.Float(), nullable=False, server_default='0.0'))

    # Create index for efficient ordering
    op.create_index('ix_task_position', 'task', ['position'])

    # Initialize position values for existing tasks
    # Group by parent_id and assign positions based on created_at order
    op.execute("""
        WITH ranked_tasks AS (
            SELECT
                id,
                ROW_NUMBER() OVER (
                    PARTITION BY COALESCE(parent_id::text, 'root')
                    ORDER BY created_at
                ) * 10.0 as new_position
            FROM task
        )
        UPDATE task
        SET position = ranked_tasks.new_position
        FROM ranked_tasks
        WHERE task.id = ranked_tasks.id
    """)


def downgrade() -> None:
    """Remove position field from task table."""

    op.drop_index('ix_task_position', 'task')
    op.drop_column('task', 'position')
