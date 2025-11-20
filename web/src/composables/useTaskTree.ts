/**
 * Task tree utilities
 *
 * Functions for tree structure management, visibility checks, and expanded state persistence
 */

import type { Task } from '@/types'

/**
 * Load expanded keys from localStorage
 */
export function loadExpandedKeys(): string[] {
  try {
    const saved = localStorage.getItem('task-tree-expanded-keys')
    if (saved) {
      return JSON.parse(saved)
    }
  } catch (error) {
    console.error('Failed to load expanded keys:', error)
  }
  return []
}

/**
 * Save expanded keys to localStorage
 */
export function saveExpandedKeys(keys: string[]): void {
  try {
    localStorage.setItem('task-tree-expanded-keys', JSON.stringify(keys))
  } catch (error) {
    console.error('Failed to save expanded keys:', error)
  }
}

/**
 * Check if ancestorTask is an ancestor of descendantTask
 */
export function isAncestorOf(ancestorId: string, descendantId: string, tasks: Task[]): boolean {
  let currentId: string | undefined = descendantId
  while (currentId) {
    const task = tasks.find(t => t.id === currentId)
    if (!task) break
    if (task.parent_id === ancestorId) return true
    currentId = task.parent_id
  }
  return false
}

/**
 * Check if task is visible in current tree (all parent tasks are expanded)
 */
export function isTaskVisible(taskId: string, tasks: Task[], expandedKeys: string[]): boolean {
  let currentId: string | undefined = taskId
  while (currentId) {
    const task = tasks.find(t => t.id === currentId)
    if (!task) break

    // Check if parent task is expanded
    if (task.parent_id) {
      const parentExpanded = expandedKeys.includes(task.parent_id)
      if (!parentExpanded) {
        return false // Parent not expanded, current task is not visible
      }
    }

    currentId = task.parent_id
  }
  return true // All parent tasks are expanded, current task is visible
}

/**
 * Find nearest visible ancestor of selected task
 */
export function findNearestVisibleAncestor(taskId: string, tasks: Task[], expandedKeys: string[]): string | null {
  let currentId: string | undefined = taskId
  const task = tasks.find(t => t.id === currentId)
  if (!task || !task.parent_id) return null

  currentId = task.parent_id
  while (currentId) {
    const parentTask = tasks.find(t => t.id === currentId)
    if (!parentTask) break

    // Check if this parent task is visible
    if (isTaskVisible(currentId, tasks, expandedKeys)) {
      return currentId
    }

    currentId = parentTask.parent_id
  }
  return null
}

/**
 * Get all descendant IDs of a task (recursively)
 */
export function getAllDescendantIds(taskId: string, tasks: Task[]): Set<string> {
  const descendants = new Set<string>()
  const addDescendants = (id: string) => {
    const children = tasks.filter(t => t.parent_id === id)
    children.forEach(child => {
      descendants.add(child.id)
      addDescendants(child.id)
    })
  }
  addDescendants(taskId)
  return descendants
}

/**
 * Build task label with status icon and working indicator
 */
export function buildTaskLabel(
  task: Task,
  isLocked: boolean,
  getStatusIconFn: (status: string) => string
): string {
  const isWorking = task.execution_state === 'working'

  // Locked tasks: show red circle slash instead of status icon
  let icon = isLocked ? '🚫' : getStatusIconFn(task.status)

  if (isWorking) {
    // Working task: add working icon
    return `${icon} ⏱️ ${task.title}`
  }

  return `${icon} ${task.title}`
}
