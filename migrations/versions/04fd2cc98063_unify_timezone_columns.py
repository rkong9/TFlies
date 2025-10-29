"""unify_timezone_columns

Revision ID: 04fd2cc98063
Revises: 9c032e296b34
Create Date: 2025-11-10 16:15:45.934787

"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision: str = '04fd2cc98063'
down_revision: Union[str, Sequence[str], None] = '9c032e296b34'
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """Upgrade schema: unify all timestamp columns to 'with time zone' (UTC)."""

    # Convert created_at and updated_at columns from 'without time zone' to 'with time zone'
    # These columns currently store UTC values without timezone markers
    # We explicitly mark them as UTC during conversion

    # Table: project
    op.execute("""
        ALTER TABLE project
        ALTER COLUMN created_at TYPE timestamp with time zone
        USING created_at AT TIME ZONE 'UTC'
    """)

    op.execute("""
        ALTER TABLE project
        ALTER COLUMN updated_at TYPE timestamp with time zone
        USING updated_at AT TIME ZONE 'UTC'
    """)

    # Table: task
    op.execute("""
        ALTER TABLE task
        ALTER COLUMN created_at TYPE timestamp with time zone
        USING created_at AT TIME ZONE 'UTC'
    """)

    op.execute("""
        ALTER TABLE task
        ALTER COLUMN updated_at TYPE timestamp with time zone
        USING updated_at AT TIME ZONE 'UTC'
    """)

    # Table: timeslice
    op.execute("""
        ALTER TABLE timeslice
        ALTER COLUMN created_at TYPE timestamp with time zone
        USING created_at AT TIME ZONE 'UTC'
    """)

    op.execute("""
        ALTER TABLE timeslice
        ALTER COLUMN updated_at TYPE timestamp with time zone
        USING updated_at AT TIME ZONE 'UTC'
    """)

    # Note: project.start_at, project.end_at, task.due_at, timeslice.start_at, timeslice.end_at
    # are already 'timestamp with time zone', so no schema change needed.
    # However, application code should ensure future writes store UTC values.


def downgrade() -> None:
    """Downgrade schema: revert to mixed timezone types (not recommended)."""

    # Revert to 'without time zone' for created_at and updated_at columns
    # This removes timezone information, keeping only the UTC timestamp values

    # Table: project
    op.execute("""
        ALTER TABLE project
        ALTER COLUMN created_at TYPE timestamp without time zone
        USING created_at AT TIME ZONE 'UTC'
    """)

    op.execute("""
        ALTER TABLE project
        ALTER COLUMN updated_at TYPE timestamp without time zone
        USING updated_at AT TIME ZONE 'UTC'
    """)

    # Table: task
    op.execute("""
        ALTER TABLE task
        ALTER COLUMN created_at TYPE timestamp without time zone
        USING created_at AT TIME ZONE 'UTC'
    """)

    op.execute("""
        ALTER TABLE task
        ALTER COLUMN updated_at TYPE timestamp without time zone
        USING updated_at AT TIME ZONE 'UTC'
    """)

    # Table: timeslice
    op.execute("""
        ALTER TABLE timeslice
        ALTER COLUMN created_at TYPE timestamp without time zone
        USING created_at AT TIME ZONE 'UTC'
    """)

    op.execute("""
        ALTER TABLE timeslice
        ALTER COLUMN updated_at TYPE timestamp without time zone
        USING updated_at AT TIME ZONE 'UTC'
    """)

    # Note: This downgrade does NOT revert project.start_at, project.end_at, task.due_at,
    # timeslice.start_at, timeslice.end_at to 'without time zone', as they should remain
    # 'with time zone' per original schema.
