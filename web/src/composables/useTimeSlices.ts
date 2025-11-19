/**
 * 时间片管理 Composable
 *
 * 提供统一的时间片管理逻辑，供桌面端和移动端共享使用
 * 核心职责：
 * 1. 管理当前正在进行的时间片状态
 * 2. 创建新时间片（开始工作）
 * 3. 更新时间片（停止工作）
 * 4. 恢复时间片（页面加载时）
 * 5. 任务互斥检查
 */

import { ref } from 'vue'
import { timeSlicesApi } from '@/api/timeSlices'
import { tasksApi } from '@/api/tasks'
import type { Task, TimeSliceCreate } from '@/types'

// 当前时间片的精简信息
export interface CurrentTimeSlice {
  id: string
  start_at: string
  task_id: string
}

// 停止工作时的配置选项
export interface StopWorkOptions {
  efficiency_score?: number  // 效率评分（1-5），默认 3
  note?: string              // 备注，默认空
}

export const useTimeSlices = () => {
  // 当前正在进行的时间片
  const currentTimeSlice = ref<CurrentTimeSlice | null>(null)

  /**
   * 从任务数据中恢复时间片状态
   * 用于页面加载时恢复正在进行的工作
   */
  const recoverTimeSlice = (task: Task): CurrentTimeSlice | null => {
    if (task.execution_state !== 'working') {
      return null
    }

    // 查找未完成的时间片（end_at 为 null）
    const ongoingSlice = task.time_slices?.find(slice => slice.end_at === null)

    if (ongoingSlice) {
      const recovered: CurrentTimeSlice = {
        id: ongoingSlice.id,
        start_at: ongoingSlice.start_at,
        task_id: task.id
      }
      currentTimeSlice.value = recovered
      return recovered
    }

    return null
  }

  /**
   * 检查是否有其他任务正在工作
   * @returns 返回正在工作的任务，如果没有则返回 null
   */
  const findWorkingTask = (tasks: Task[], excludeTaskId?: string): Task | null => {
    return tasks.find(t =>
      t.execution_state === 'working' &&
      t.id !== excludeTaskId
    ) || null
  }

  /**
   * 开始工作：创建新时间片并设置任务状态为 working
   *
   * @param task 要开始工作的任务
   * @returns 成功返回创建的时间片信息，失败抛出异常
   * @throws 如果任务状态不是 in_progress，抛出错误
   */
  const startWork = async (task: Task): Promise<CurrentTimeSlice> => {
    // 检查任务状态
    if (task.status !== 'in_progress') {
      throw new Error('只有"进行中"状态的任务才能开始工作')
    }

    // 创建新时间片
    const startTime = new Date().toISOString()
    const response = await timeSlicesApi.create({
      task_id: task.id,
      start_at: startTime
    } as TimeSliceCreate)

    // 保存当前时间片
    const newTimeSlice: CurrentTimeSlice = {
      id: response.data.id,
      start_at: response.data.start_at,
      task_id: task.id
    }
    currentTimeSlice.value = newTimeSlice

    // 更新任务状态为工作中
    await tasksApi.update(task.id, { execution_state: 'working' })

    return newTimeSlice
  }

  /**
   * 停止工作：更新时间片并设置任务状态为 idle
   *
   * @param task 要停止工作的任务
   * @param options 配置选项（效率评分、备注）
   * @returns 成功返回更新的时间片 ID，失败抛出异常
   * @throws 如果当前没有进行中的时间片，抛出错误
   */
  const stopWork = async (
    task: Task,
    options: StopWorkOptions = {}
  ): Promise<string> => {
    // 如果没有 currentTimeSlice，尝试从任务数据中恢复
    if (!currentTimeSlice.value) {
      const recovered = recoverTimeSlice(task)
      if (!recovered) {
        throw new Error('当前没有进行中的时间片')
      }
    }

    // 确保时间片属于当前任务
    if (currentTimeSlice.value!.task_id !== task.id) {
      throw new Error('时间片任务不匹配')
    }

    const endTime = new Date()
    const startTime = new Date(currentTimeSlice.value!.start_at)
    const durationMs = endTime.getTime() - startTime.getTime()

    // 更新时间片
    const timeSliceId = currentTimeSlice.value!.id
    await timeSlicesApi.update(timeSliceId, {
      end_at: endTime.toISOString(),
      duration_ms: durationMs,
      efficiency_score: options.efficiency_score ?? 3, // 默认中等效率
      note: options.note ?? ''
    })

    // 清空当前时间片
    currentTimeSlice.value = null

    // 更新任务状态为空闲
    await tasksApi.update(task.id, { execution_state: 'idle' })

    return timeSliceId
  }

  /**
   * 清空当前时间片
   * 用于手动清理状态
   */
  const clearTimeSlice = () => {
    currentTimeSlice.value = null
  }

  /**
   * 获取当前时间片的工作时长（毫秒）
   */
  const getCurrentDuration = (): number => {
    if (!currentTimeSlice.value) return 0

    const now = new Date()
    const startTime = new Date(currentTimeSlice.value.start_at)
    return now.getTime() - startTime.getTime()
  }

  return {
    // 状态
    currentTimeSlice,

    // 方法
    startWork,
    stopWork,
    recoverTimeSlice,
    clearTimeSlice,
    findWorkingTask,
    getCurrentDuration
  }
}
