/**
 * 任务相关API
 */
import { apiClient } from './client'
import type { Task, TaskCreate, DeletedTask, DeletedTaskDetail } from '@/types'

export const tasksApi = {
  // 获取所有任务
  getAll: (projectId?: string) => {
    const params = projectId ? { project_id: projectId } : {}
    return apiClient.get<Task[]>('/tasks/', { params })
  },

  // 获取单个任务
  get: (id: string) => apiClient.get<Task>(`/tasks/${id}`),

  // 创建任务
  create: (data: TaskCreate) => apiClient.post<Task>('/tasks/', data),

  // 更新任务
  update: (id: string, data: Partial<TaskCreate>) =>
    apiClient.patch<Task>(`/tasks/${id}`, data),

  // 删除任务
  delete: (id: string) => apiClient.delete(`/tasks/${id}`),

  // 更新任务位置（拖拽排序）
  updatePosition: (id: string, position: number) =>
    apiClient.patch<Task>(`/tasks/${id}/position?new_position=${position}`),

  // 解锁DDL锁定的任务
  unlock: (id: string) => apiClient.post<Task>(`/tasks/${id}/unlock`),

  // 回收站相关API
  trash: {
    // 获取回收站列表
    list: () => apiClient.get<DeletedTask[]>('/tasks/trash/list'),

    // 获取删除任务详情
    get: (id: string) => apiClient.get<DeletedTaskDetail>(`/tasks/trash/${id}`),

    // 从回收站恢复
    restore: (id: string) => apiClient.post<Task>(`/tasks/trash/${id}/restore`),

    // 永久删除
    permanentDelete: (id: string) => apiClient.delete(`/tasks/trash/${id}`),

    // 清空回收站
    empty: () => apiClient.delete<{ deleted_count: number }>('/tasks/trash/empty'),
  },
}
