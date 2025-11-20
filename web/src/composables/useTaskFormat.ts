/**
 * Task formatting utilities
 *
 * Pure functions for formatting task-related data
 */

/**
 * Format duration in milliseconds to human-readable string
 * @example formatDuration(3661000) => "1h 1min 1s"
 */
export function formatDuration(ms: number): string {
  if (ms === 0) return '0s'

  const totalSeconds = Math.floor(ms / 1000)
  const days = Math.floor(totalSeconds / 86400)
  const hours = Math.floor((totalSeconds % 86400) / 3600)
  const minutes = Math.floor((totalSeconds % 3600) / 60)
  const seconds = totalSeconds % 60

  const parts: string[] = []
  if (days > 0) parts.push(`${days}d`)
  if (hours > 0) parts.push(`${hours}h`)
  if (minutes > 0) parts.push(`${minutes}min`)
  if (seconds > 0) parts.push(`${seconds}s`)

  return parts.join(' ') || '0s'
}

/**
 * Format time slice range with smart cross-day detection
 * @example
 * Same day: "11-19 10:00 - 11:30"
 * Cross-day: "11-19 23:00 - 11-20 01:00"
 */
export function formatTimeSliceRange(startAt: string, endAt: string): string {
  const start = new Date(startAt)
  const end = new Date(endAt)

  // Check if same day
  const isSameDay = start.getFullYear() === end.getFullYear() &&
                    start.getMonth() === end.getMonth() &&
                    start.getDate() === end.getDate()

  if (isSameDay) {
    // Same day: show date once + time range
    return `${start.toLocaleString('zh-CN', {
      month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit'
    })} - ${end.toLocaleTimeString('zh-CN', {
      hour: '2-digit', minute: '2-digit'
    })}`
  } else {
    // Cross-day: show full datetime for both
    return `${start.toLocaleString('zh-CN', {
      month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit'
    })} - ${end.toLocaleString('zh-CN', {
      month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit'
    })}`
  }
}

/**
 * Format date to locale string
 */
export function formatDate(date: Date | string): string {
  const d = typeof date === 'string' ? new Date(date) : date
  return d.toLocaleDateString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit'
  })
}

/**
 * Format datetime to locale string
 */
export function formatDateTime(date: Date | string): string {
  const d = typeof date === 'string' ? new Date(date) : date
  return d.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  })
}
