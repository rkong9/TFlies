import { ref, computed, onMounted } from 'vue'
import { useMessage } from 'naive-ui'
import { tasksApi } from '@/api/tasks'
import { getOrCreateDefaultProject } from '@/api'
import { useTimeSlices } from './useTimeSlices'
import type { Task, TaskCreate } from '@/types'

export const useTasks = () => {
  const message = useMessage()

  // 使用时间片管理 composable
  const {
    currentTimeSlice,
    startWork,
    stopWork,
    recoverTimeSlice,
    findWorkingTask
  } = useTimeSlices()

  // 核心状态
  const tasks = ref<Task[]>([])
  const selectedTask = ref<Task | null>(null)
  const isLoading = ref(false)
  
  // 计算属性
  const taskTree = computed(() => {
    // 构建任务树结构（简化版）
    const taskMap = new Map()
    const rootTasks: Task[] = []
    
    // 按position排序
    const sortedTasks = [...tasks.value].sort((a, b) => a.position - b.position)
    
    // 建立映射
    sortedTasks.forEach(task => {
      taskMap.set(task.id, { ...task, children: [] })
    })
    
    // 构建树结构
    sortedTasks.forEach(task => {
      if (task.parent_id && taskMap.has(task.parent_id)) {
        taskMap.get(task.parent_id).children.push(taskMap.get(task.id))
      } else {
        rootTasks.push(taskMap.get(task.id))
      }
    })
    
    return rootTasks
  })
  
  // 获取工作中的任务
  const workingTask = computed(() => {
    return tasks.value.find(t => t.execution_state === 'working')
  })
  
  // 统计数据
  const stats = computed(() => {
    const total = tasks.value.length
    const completed = tasks.value.filter(t => t.status === 'completed').length
    const pending = tasks.value.filter(t => t.status === 'pending').length
    const inProgress = tasks.value.filter(t => t.status === 'in_progress').length
    
    return {
      total,
      completed,
      pending,
      inProgress,
      workingCount: workingTask.value ? 1 : 0
    }
  })
  
  // 加载所有任务
  const loadTasks = async () => {
    try {
      isLoading.value = true

      // 确保默认项目存在
      await getOrCreateDefaultProject()

      // 加载所有任务
      const response = await tasksApi.getAll()
      tasks.value = response.data

      console.log('📋 任务数据已加载:', tasks.value.length, '个任务')

      // 检查并恢复正在工作的任务的时间片
      const working = tasks.value.find(t => t.execution_state === 'working')
      if (working) {
        const recovered = recoverTimeSlice(working)
        if (recovered) {
          console.log('⏱️ 恢复进行中的时间片:', recovered)
        } else {
          // 数据不一致，自动修正
          console.warn('⚠️ 检测到数据不一致：任务状态为working但没有进行中的时间片')
          await tasksApi.update(working.id, { execution_state: 'idle' })
        }
      }

    } catch (error) {
      console.error('❌ 加载任务失败:', error)
      message.error('加载任务失败')
    } finally {
      isLoading.value = false
    }
  }
  
  // 创建任务
  const createTask = async (taskData: TaskCreate) => {
    try {
      console.log('🆕 创建任务:', taskData)
      await tasksApi.create(taskData)
      message.success('任务创建成功')
      
      // 重新加载数据
      await loadTasks()
      
    } catch (error) {
      console.error('❌ 创建任务失败:', error)
      message.error('创建任务失败')
    }
  }
  
  // 更新任务
  const updateTask = async (taskId: string, updates: Partial<Task>) => {
    try {
      console.log('📝 更新任务:', taskId, updates)
      await tasksApi.update(taskId, updates)
      message.success('任务更新成功')
      
      // 重新加载数据
      await loadTasks()
      
    } catch (error) {
      console.error('❌ 更新任务失败:', error)
      message.error('更新任务失败')
    }
  }
  
  // 删除任务
  const deleteTask = async (taskId: string) => {
    try {
      console.log('🗑️ 删除任务:', taskId)
      await tasksApi.delete(taskId)
      message.success('任务删除成功')
      
      // 重新加载数据
      await loadTasks()
      
    } catch (error) {
      console.error('❌ 删除任务失败:', error)
      message.error('删除任务失败')
    }
  }
  
  // 开始/停止工作
  const toggleTaskWork = async (task: Task) => {
    try {
      if (task.execution_state === 'working') {
        // 停止工作：使用统一的 stopWork 方法
        await stopWork(task, {
          efficiency_score: 3, // 移动端默认中等效率
          note: '移动端快速记录'
        })
        message.success('工作已停止')

      } else {
        // 开始工作：使用统一的 startWork 方法

        // 检查任务状态（startWork 内部也会检查，这里提前检查可以给出友好提示）
        if (task.status !== 'in_progress') {
          message.warning('请先将任务状态改为"进行中"')
          return
        }

        // 停止其他正在工作的任务
        const working = findWorkingTask(tasks.value, task.id)
        if (working) {
          await toggleTaskWork(working) // 递归停止其他任务
        }

        // 开始新任务
        await startWork(task)
        message.success('开始工作计时')
      }

      // 重新加载数据
      await loadTasks()

    } catch (error) {
      console.error('❌ 切换任务状态失败:', error)
      message.error(error instanceof Error ? error.message : '操作失败')
    }
  }
  
  // 选择任务
  const selectTask = (task: Task | null) => {
    selectedTask.value = task
    console.log('🎯 选中任务:', task?.title || '无')
  }
  
  // 根据ID查找任务
  const findTaskById = (taskId: string): Task | null => {
    return tasks.value.find(t => t.id === taskId) || null
  }
  
  // 获取任务的状态图标
  const getTaskStatusIcon = (task: Task): string => {
    if (task.execution_state === 'working') return '⏱️'
    
    switch (task.status) {
      case 'pending': return '⬜'
      case 'in_progress': return '⏳'
      case 'completed': return '✅'
      case 'cancelled': return '❌'
      default: return '⬜'
    }
  }
  
  // 获取优先级颜色
  const getPriorityColor = (priority: number): string => {
    switch (priority) {
      case 1: return '#9333ea' // 紫色 - 最高优先级
      case 2: return '#ef4444' // 红色 - 高优先级
      case 3: return '#eab308' // 黄色 - 中等优先级
      case 4: return '#3b82f6' // 蓝色 - 低优先级
      case 5: return '#9ca3af' // 灰色 - 最低优先级
      default: return '#9ca3af'
    }
  }
  
  // 生命周期
  onMounted(() => {
    loadTasks()
  })
  
  return {
    // 状态
    tasks,
    selectedTask,
    isLoading,
    currentTimeSlice, // 导出当前时间片状态

    // 计算属性
    taskTree,
    workingTask,
    stats,

    // 方法
    loadTasks,
    createTask,
    updateTask,
    deleteTask,
    toggleTaskWork,
    selectTask,
    findTaskById,

    // 工具方法
    getTaskStatusIcon,
    getPriorityColor
  }
}