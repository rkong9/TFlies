"""add_deleted_task_table

Revision ID: 0236a1bfc28b
Revises: 04fd2cc98063
Create Date: 2025-11-10 16:59:47.465342

"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects.postgresql import JSONB, UUID


# revision identifiers, used by Alembic.
revision: str = '0236a1bfc28b'
down_revision: Union[str, Sequence[str], None] = '04fd2cc98063'
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """Create deleted_task table for trash/recycle bin functionality."""
    op.create_table(
        'deletedtask',
        sa.Column('id', UUID(as_uuid=True), primary_key=True, nullable=False),
        sa.Column('task_id', UUID(as_uuid=True), nullable=False, comment='Original task ID'),
        sa.Column('deleted_at', sa.DateTime(timezone=True), nullable=False, comment='When the task was deleted'),
        sa.Column('expiry_at', sa.DateTime(timezone=True), nullable=False, comment='When this record should be permanently deleted'),
        sa.Column('snapshot', JSONB, nullable=False, comment='Complete task tree snapshot in JSON format'),
        sa.Column('deletion_metadata', JSONB, nullable=True, comment='Additional metadata (deletion reason, stats, etc.)'),
    )

    # Create indexes for efficient querying
    op.create_index('ix_deletedtask_task_id', 'deletedtask', ['task_id'])
    op.create_index('ix_deletedtask_expiry_at', 'deletedtask', ['expiry_at'])


def downgrade() -> None:
    """Drop deleted_task table."""
    op.drop_index('ix_deletedtask_expiry_at', 'deletedtask')
    op.drop_index('ix_deletedtask_task_id', 'deletedtask')
    op.drop_table('deletedtask')

