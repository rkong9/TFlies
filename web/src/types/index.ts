/**
 * 类型定义
 */

export type TaskStatus = 'todo' | 'in_progress' | 'suspended' | 'completed'

export type ExecutionState = 'idle' | 'working'

export interface Project {
  id: string
  name: string
  description?: string
  planned_time_ms: number
  start_at?: string
  end_at?: string
  created_at: string
  updated_at: string
  tasks: Task[]
  total_logged_ms: number
}

export interface Task {
  id: string
  project_id: string
  parent_id?: string
  title: string
  description?: string
  priority: number
  due_at?: string
  is_ddl: boolean
  estimated_time_ms: number | null
  status: TaskStatus
  execution_state: ExecutionState
  position: number
  created_at: string
  updated_at: string
  time_slices: TimeSlice[]
  children?: Task[]
  total_logged_ms: number
}

export interface TimeSlice {
  id: string
  task_id: string
  start_at: string
  end_at: string | null
  duration_ms: number | null
  efficiency_score: number | null
  note?: string
  created_at: string
  updated_at: string
}

export interface ProjectCreate {
  name: string
  description?: string
  planned_time_ms?: number
  start_at?: string
  end_at?: string
}

export interface TaskCreate {
  project_id: string
  parent_id?: string
  title: string
  description?: string
  priority?: number
  due_at?: string
  is_ddl?: boolean
  estimated_time_ms?: number | null
  status?: TaskStatus
  execution_state?: ExecutionState
}

export interface TimeSliceCreate {
  task_id: string
  start_at: string
  end_at: string | null
  duration_ms?: number | null
  efficiency_score: number | null
  note?: string | null
}

// 回收站相关类型
export interface DeletedTask {
  id: string
  task_id: string
  deleted_at: string
  expiry_at: string
  title: string
  project_id: string
  total_descendants: number
}

export interface DeletedTaskDetail {
  id: string
  task_id: string
  deleted_at: string
  expiry_at: string
  snapshot: {
    task: Task
    children: any[]
    time_slices: TimeSlice[]
  }
  deletion_metadata?: {
    title: string
    project_id: string
    total_descendants: number
  }
}
