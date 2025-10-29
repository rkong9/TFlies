/**
 * 时间片相关API
 */
import { apiClient } from './client'
import type { TimeSlice, TimeSliceCreate } from '@/types'

export const timeSlicesApi = {
  // 获取所有时间片
  getAll: (taskId?: string) => {
    const params = taskId ? { task_id: taskId } : {}
    return apiClient.get<TimeSlice[]>('/time-slices/', { params })
  },

  // 获取单个时间片
  get: (id: string) => apiClient.get<TimeSlice>(`/time-slices/${id}`),

  // 创建时间片
  create: (data: TimeSliceCreate) => apiClient.post<TimeSlice>('/time-slices/', data),

  // 更新时间片
  update: (id: string, data: Partial<TimeSliceCreate>) =>
    apiClient.patch<TimeSlice>(`/time-slices/${id}`, data),

  // 删除时间片
  delete: (id: string) => apiClient.delete(`/time-slices/${id}`),
}
