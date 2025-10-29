/**
 * 项目相关API
 */
import { apiClient } from './client'
import type { Project, ProjectCreate } from '@/types'

export const projectsApi = {
  // 获取所有项目
  getAll: () => apiClient.get<Project[]>('/projects/'),

  // 获取单个项目
  get: (id: string) => apiClient.get<Project>(`/projects/${id}`),

  // 创建项目
  create: (data: ProjectCreate) => apiClient.post<Project>('/projects/', data),

  // 更新项目
  update: (id: string, data: Partial<ProjectCreate>) =>
    apiClient.patch<Project>(`/projects/${id}`, data),

  // 删除项目
  delete: (id: string) => apiClient.delete(`/projects/${id}`),
}
