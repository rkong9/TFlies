/**
 * Task-related constants and mappings
 */
import type { TaskStatus } from '@/types'

/**
 * Status display options for dropdowns
 */
export const statusOptions = [
  { label: '⬜ 待办', value: 'todo' },
  { label: '⏳ 进行中', value: 'in_progress' },
  { label: '⏸️ 挂起', value: 'suspended' },
  { label: '✅ 完成', value: 'completed' },
]

/**
 * Get status icon emoji
 */
export function getStatusIcon(status: string): string {
  const icons: Record<string, string> = {
    todo: '⬜',
    in_progress: '⏳',
    suspended: '⏸️',
    completed: '✅',
  }
  return icons[status] || '•'
}

/**
 * Get status color
 */
export function getStatusColor(status: TaskStatus): string {
  const colors: Record<TaskStatus, string> = {
    todo: '#9ca3af',          // 灰色
    in_progress: '#60a5fa',   // 浅蓝色
    suspended: '#fbbf24',     // 浅黄色
    completed: '#34d399',     // 浅绿色
  }
  return colors[status] || '#9ca3af'
}

/**
 * Get status label in Chinese
 */
export function getStatusLabel(status: TaskStatus): string {
  const labels: Record<TaskStatus, string> = {
    todo: '待办',
    in_progress: '进行中',
    suspended: '挂起',
    completed: '完成',
  }
  return labels[status] || status
}

/**
 * Get execution state label
 */
export function getExecutionLabel(execution_state: string): string {
  return execution_state === 'working' ? '工作中' : '空闲'
}

/**
 * Get color based on time usage rate percentage
 */
export function getUsageRateColor(usageRate: number): string {
  if (usageRate < 40) return '#22c55e'      // 绿色：效率很高
  if (usageRate < 80) return '#eab308'      // 黄色：正常进度
  if (usageRate < 120) return '#f97316'     // 橙色：接近/轻微超时
  if (usageRate < 240) return '#ef4444'     // 红色：明显超时
  return '#000000'                          // 黑色：严重超时
}

/**
 * Get usage status text description
 */
export function getUsageStatusText(usageRate: number, hasEstimate: boolean, actualTime: number): string {
  if (actualTime === 0) {
    return hasEstimate ? '🆕 尚未开始' : '🆕 尚未开始'
  }

  if (!hasEstimate) {
    return '⚠️ 未设定预期时间'
  }

  if (usageRate < 40) return '⚡ 效率很高'
  if (usageRate < 80) return '✅ 进度正常'
  if (usageRate < 120) return '⚠️ 接近预期'
  if (usageRate < 240) return '🔴 时间超支'
  return '💀 严重超时'
}

/**
 * Priority background colors
 */
export function getPriorityColor(priority: number): string {
  const colors: Record<number, string> = {
    1: 'rgba(239, 68, 68, 0.08)',   // 红色背景 - 最高优先级
    2: 'rgba(249, 115, 22, 0.08)',  // 橙色背景 - 高优先级
    3: 'rgba(234, 179, 8, 0.08)',   // 黄色背景 - 中等优先级
    4: 'rgba(59, 130, 246, 0.08)',  // 蓝色背景 - 低优先级
    5: 'rgba(156, 163, 175, 0.08)', // 灰色背景 - 最低优先级
  }
  return colors[priority] ?? colors[3]!
}
