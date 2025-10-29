/**
 * API统一导出
 */
export * from './client'
export * from './projects'
export * from './tasks'
export * from './timeSlices'

/**
 * 获取或创建默认项目
 */
import { projectsApi } from './projects'
import type { Project } from '@/types'

const DEFAULT_PROJECT_NAME = '我的任务'

let cachedDefaultProject: Project | null = null

export async function getOrCreateDefaultProject(): Promise<Project> {
  // 如果已有缓存，直接返回
  if (cachedDefaultProject) {
    return cachedDefaultProject
  }

  try {
    // 获取所有项目
    const response = await projectsApi.getAll()
    const projects = response.data

    // 查找默认项目
    let defaultProject = projects.find(p => p.name === DEFAULT_PROJECT_NAME)

    // 如果不存在，创建一个
    if (!defaultProject) {
      const createResponse = await projectsApi.create({
        name: DEFAULT_PROJECT_NAME,
        description: '默认项目，用于管理所有任务',
        planned_time_ms: 0,
      })
      defaultProject = createResponse.data
    }

    // 缓存并返回
    cachedDefaultProject = defaultProject
    return defaultProject
  } catch (error) {
    console.error('Failed to get or create default project:', error)
    throw error
  }
}

/**
 * 清除默认项目缓存（用于刷新时）
 */
export function clearDefaultProjectCache() {
  cachedDefaultProject = null
}
