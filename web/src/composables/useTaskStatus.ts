/**
 * Task status and calculation utilities
 *
 * Functions for task status checks, DDL validation, time calculations, and efficiency metrics
 */

import type { Task } from '@/types'
import { formatDuration } from './useTaskFormat'

/**
 * Priority options for task selection
 */
export const priorityOptions = [
  { label: 'P1 - 最高', value: 1 },
  { label: 'P2 - 高', value: 2 },
  { label: 'P3 - 中', value: 3 },
  { label: 'P4 - 低', value: 4 },
  { label: 'P5 - 最低', value: 5 },
]

/**
 * Get effective due date (considering parent task inheritance)
 */
export function getEffectiveDueAt(task: Task, tasks: Task[]): string | null {
  // If task has its own due date, return it
  if (task.due_at) {
    return task.due_at
  }

  // Otherwise, look up parent task's due date
  if (task.parent_id) {
    const parent = tasks.find(t => t.id === task.parent_id)
    if (parent) {
      return getEffectiveDueAt(parent, tasks)
    }
  }

  return null
}

/**
 * Get effective DDL mode (considering parent task inheritance)
 */
export function getEffectiveIsDDL(task: Task, tasks: Task[]): boolean {
  // If task has DDL enabled, return true
  if (task.is_ddl) {
    return true
  }

  // Otherwise, look up parent task's DDL setting
  if (task.parent_id) {
    const parent = tasks.find(t => t.id === task.parent_id)
    if (parent) {
      return getEffectiveIsDDL(parent, tasks)
    }
  }

  return false
}

/**
 * Check if task is locked (DDL expired)
 */
export function isTaskLocked(task: Task, tasks: Task[]): boolean {
  const effectiveDueAt = getEffectiveDueAt(task, tasks)
  const effectiveIsDDL = getEffectiveIsDDL(task, tasks)

  if (!effectiveDueAt || !effectiveIsDDL) {
    return false
  }

  const dueDate = new Date(effectiveDueAt)
  const now = new Date()
  return now > dueDate
}

/**
 * Check if task is overdue (but not necessarily locked)
 */
export function isTaskOverdue(task: Task, tasks: Task[]): boolean {
  const effectiveDueAt = getEffectiveDueAt(task, tasks)
  if (!effectiveDueAt) {
    return false
  }

  const dueDate = new Date(effectiveDueAt)
  const now = new Date()
  return now > dueDate
}

/**
 * Calculate total time of all child tasks recursively
 */
export function calculateChildrenTotalTime(taskId: string, tasks: Task[]): number {
  const children = tasks.filter(t => t.parent_id === taskId)
  let total = 0

  for (const child of children) {
    // Child task's own time
    total += child.total_logged_ms
    // Recursively calculate child's children
    total += calculateChildrenTotalTime(child.id, tasks)
  }

  return total
}

/**
 * Check if task has children
 */
export function hasChildren(taskId: string, tasks: Task[]): boolean {
  return tasks.some(t => t.parent_id === taskId)
}

/**
 * Get effective estimated time (considering child tasks)
 */
export function getEffectiveEstimatedTime(task: Task, tasks: Task[]): number | null {
  // If task has its own estimated time, return it
  if (task.estimated_time_ms !== null) {
    return task.estimated_time_ms
  }

  // Otherwise, recursively calculate sum of all child tasks' estimated time
  const children = tasks.filter(t => t.parent_id === task.id)
  if (children.length === 0) {
    return null // No children, return null
  }

  let total = 0
  let hasAnyEstimate = false

  for (const child of children) {
    const childEstimate = getEffectiveEstimatedTime(child, tasks)
    if (childEstimate !== null) {
      total += childEstimate
      hasAnyEstimate = true
    }
  }

  return hasAnyEstimate ? total : null
}

/**
 * Calculate time usage rate (actual time / estimated time)
 * Returns percentage value
 */
export function calculateTimeUsageRate(task: Task, tasks: Task[]): number {
  const estimatedTime = getEffectiveEstimatedTime(task, tasks)
  const actualTime = task.total_logged_ms + calculateChildrenTotalTime(task.id, tasks)

  // Special case 1: Haven't started yet
  if (actualTime === 0) {
    return 0  // 0% → green
  }

  // Special case 2: Has actual time but no estimated time set
  if (estimatedTime === null || estimatedTime === 0) {
    return 100  // 100% → orange
  }

  // Normal case: actual time / estimated time
  return (actualTime / estimatedTime) * 100
}

/**
 * Check if task has estimated time set
 */
export function hasEstimatedTime(task: Task, tasks: Task[]): boolean {
  const estimatedTime = getEffectiveEstimatedTime(task, tasks)
  return estimatedTime !== null && estimatedTime > 0
}

/**
 * Generate usage rate text display with detailed time info
 */
export function getUsageRateText(task: Task, tasks: Task[]): string {
  const usageRate = calculateTimeUsageRate(task, tasks)
  const hasEstimate = hasEstimatedTime(task, tasks)
  const actualTime = task.total_logged_ms + calculateChildrenTotalTime(task.id, tasks)
  const estimatedTime = getEffectiveEstimatedTime(task, tasks)

  if (actualTime === 0) {
    return hasEstimate ? '未开始' : '未开始'
  }

  if (!hasEstimate) {
    return `${usageRate.toFixed(1)}% (未设定预期)`
  }

  return `${usageRate.toFixed(1)}% (${formatDuration(actualTime)} / ${formatDuration(estimatedTime!)})`
}

/**
 * Calculate time efficiency (estimated time / actual time, higher is better)
 * Returns percentage value
 */
export function calculateTimeEfficiency(task: Task, tasks: Task[]): number {
  // Get effective estimated time
  const effectiveEstimatedTime = getEffectiveEstimatedTime(task, tasks)
  if (effectiveEstimatedTime === null) return 0

  // Calculate actual time = own time + all child tasks' time
  const totalActualTime = task.total_logged_ms + calculateChildrenTotalTime(task.id, tasks)

  if (totalActualTime === 0) return 0
  return (effectiveEstimatedTime / totalActualTime) * 100
}

/**
 * Calculate work efficiency (weighted average of time slice efficiency scores)
 * Recursively calculates efficiency for task and all child tasks
 * Returns score from 0-5
 */
export function calculateWorkEfficiency(task: Task, tasks: Task[]): number {
  let totalWeightedScore = 0
  let totalDuration = 0

  // Calculate own time slices
  for (const slice of task.time_slices) {
    if (slice.duration_ms !== null && slice.efficiency_score !== null) {
      totalWeightedScore += slice.duration_ms * slice.efficiency_score
      totalDuration += slice.duration_ms
    }
  }

  // Recursively calculate all child tasks' efficiency
  const children = tasks.filter(t => t.parent_id === task.id)
  for (const child of children) {
    const childEfficiency = calculateWorkEfficiency(child, tasks)
    const childDuration = child.total_logged_ms + calculateChildrenTotalTime(child.id, tasks)
    if (childDuration > 0) {
      totalWeightedScore += childDuration * childEfficiency
      totalDuration += childDuration
    }
  }

  if (totalDuration === 0) return 0
  return totalWeightedScore / totalDuration
}
