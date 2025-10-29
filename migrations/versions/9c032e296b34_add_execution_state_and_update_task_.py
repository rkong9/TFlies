"""Add execution_state and update task_status

Revision ID: 9c032e296b34
Revises: 9058b5fbf03e
Create Date: 2025-11-10 11:57:42.911693

"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision: str = '9c032e296b34'
down_revision: Union[str, Sequence[str], None] = '9058b5fbf03e'
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """Upgrade schema."""
    # Create new execution_state enum (using lowercase)
    op.execute("CREATE TYPE execution_state AS ENUM ('IDLE', 'WORKING')")

    # Add execution_state column with default value
    op.add_column('task', sa.Column('execution_state', sa.Enum('IDLE', 'WORKING', name='execution_state'), nullable=False, server_default='IDLE'))

    # Update task_status enum: remove PAUSED and CANCELLED, add SUSPENDED
    # Step 1: Convert status column to text temporarily
    op.execute("ALTER TABLE task ALTER COLUMN status TYPE text")

    # Step 2: Update existing PAUSED/CANCELLED tasks to SUSPENDED
    op.execute("UPDATE task SET status = 'SUSPENDED' WHERE status IN ('PAUSED', 'CANCELLED')")

    # Step 3: Create new enum type
    op.execute("CREATE TYPE task_status_new AS ENUM ('TODO', 'IN_PROGRESS', 'SUSPENDED', 'COMPLETED')")

    # Step 4: Convert status column to new enum
    op.execute("ALTER TABLE task ALTER COLUMN status TYPE task_status_new USING status::task_status_new")

    # Step 5: Drop old enum and rename new one
    op.execute("DROP TYPE task_status")
    op.execute("ALTER TYPE task_status_new RENAME TO task_status")

    # Add check constraint
    op.create_check_constraint(
        'ck_execution_only_when_in_progress',
        'task',
        "(status = 'IN_PROGRESS') OR (execution_state = 'IDLE')"
    )


def downgrade() -> None:
    """Downgrade schema."""
    # Drop check constraint
    op.drop_constraint('ck_execution_only_when_in_progress', 'task', type_='check')

    # Restore old task_status enum
    op.execute("CREATE TYPE task_status_old AS ENUM ('TODO', 'IN_PROGRESS', 'PAUSED', 'COMPLETED', 'CANCELLED')")
    op.execute("UPDATE task SET status = 'PAUSED' WHERE status = 'SUSPENDED'")
    op.execute("ALTER TABLE task ALTER COLUMN status TYPE task_status_old USING status::text::task_status_old")
    op.execute("DROP TYPE task_status")
    op.execute("ALTER TYPE task_status_old RENAME TO task_status")

    # Drop execution_state column and enum
    op.drop_column('task', 'execution_state')
    op.execute("DROP TYPE execution_state")
