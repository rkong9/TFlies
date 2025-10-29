<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import { useMessage, useDialog } from 'naive-ui'
import {
  NLayout,
  NLayoutHeader,
  NLayoutContent,
  NButton,
  NSpace,
  NTree,
  NTreeSelect,
  NInput,
  NModal,
  NForm,
  NFormItem,
  NInputNumber,
  NProgress,
  NStatistic,
  NGrid,
  NGridItem,
  NCard,
  NSelect,
  NPopconfirm,
  NDropdown,
  NDatePicker,
  NCheckbox,
} from 'naive-ui'
import type { TreeOption } from 'naive-ui'
import { tasksApi } from '@/api/tasks'
import { timeSlicesApi } from '@/api/timeSlices'
import { getOrCreateDefaultProject } from '@/api'
import type { Project, Task, TaskCreate, TimeSlice, TimeSliceCreate, TaskStatus } from '@/types'
import TrashView from './TrashView.vue'
import DailyEfficiencyChart from '@/components/DailyEfficiencyChart.vue'
import WeeklyBarChart from '@/components/WeeklyBarChart.vue'
import ActivityHeatmap from '@/components/ActivityHeatmap.vue'

const message = useMessage()
const dialog = useDialog()
const tasks = ref<Task[]>([])
const selectedTask = ref<Task | null>(null)
const selectedTimeSlice = ref<TimeSlice | null>(null)
const dailyChartRef = ref<InstanceType<typeof DailyEfficiencyChart> | null>(null)
const showCreateModal = ref(false)
const showEditModal = ref(false)
const showTimeSliceModal = ref(false)
const showEditTimeSliceModal = ref(false)
const showTrash = ref(false)
const searchKeyword = ref('')
const currentTimeSlice = ref<{ id: string; start_at: string; task_id: string } | null>(null)
const timeSliceForm = ref({
  efficiency_score: 3,
  note: '',
})
const editTimeSliceForm = ref({
  id: '',
  start_at: '',
  end_at: '',
  efficiency_score: 3,
  note: '',
})

// 任务树展开状态（从localStorage恢复，默认全部折叠）
const expandedKeys = ref<string[]>([])

// 从localStorage恢复展开状态
const loadExpandedKeys = () => {
  try {
    const saved = localStorage.getItem('task-tree-expanded-keys')
    if (saved) {
      expandedKeys.value = JSON.parse(saved)
    }
  } catch (error) {
    console.error('Failed to load expanded keys:', error)
  }
}

// 保存展开状态到localStorage
const saveExpandedKeys = (keys: string[]) => {
  try {
    localStorage.setItem('task-tree-expanded-keys', JSON.stringify(keys))
  } catch (error) {
    console.error('Failed to save expanded keys:', error)
  }
}

// 处理展开状态变化
const handleExpandedKeysChange = (keys: string[]) => {
  expandedKeys.value = keys
  saveExpandedKeys(keys)
}

// 右键菜单相关状态
const showContextMenu = ref(false)
const contextMenuX = ref(0)
const contextMenuY = ref(0)
const contextMenuTask = ref<Task | null>(null)

// 拖拽状态：记录当前被拖拽的节点
const draggingNode = ref<any>(null)

// 右键菜单选项（根据任务锁定状态动态生成）
const contextMenuOptions = computed(() => {
  const task = contextMenuTask.value
  const locked = task ? isTaskLocked(task) : false

  return [
    {
      label: '创建子任务',
      key: 'create-child',
      icon: () => '➕',
      disabled: locked,
    },
    {
      label: locked ? '编辑任务 🔒' : '编辑任务',
      key: 'edit',
      icon: () => '✏️',
      disabled: locked,
    },
    {
      label: locked ? '删除任务 🔒' : '删除任务',
      key: 'delete',
      icon: () => '🗑️',
      disabled: locked,
    },
  ]
})

// 处理右键菜单点击
const handleContextMenuSelect = (key: string) => {
  if (!contextMenuTask.value) return

  switch (key) {
    case 'create-child':
      // 打开创建任务弹窗，并自动设置父任务
      newTask.value = {
        title: '',
        description: '',
        estimated_time_ms: 1,
        parent_id: contextMenuTask.value.id,
      }
      showCreateModal.value = true
      break
    case 'edit':
      handleEditTask(contextMenuTask.value)
      break
    case 'delete':
      handleDeleteTask(contextMenuTask.value.id)
      break
  }

  showContextMenu.value = false
  contextMenuTask.value = null
}

// 处理任务节点右键事件
const handleNodeContextMenu = (e: MouseEvent, task: Task) => {
  e.preventDefault()
  contextMenuX.value = e.clientX
  contextMenuY.value = e.clientY
  contextMenuTask.value = task
  showContextMenu.value = true
}

const newTask = ref({
  title: '',
  description: '',
  due_at: null as string | null,
  is_ddl: false,
  estimated_time_ms: 1,
  parent_id: undefined as string | undefined,
})
const editTask = ref({
  id: '',
  title: '',
  description: '',
  due_at: null as string | null,
  is_ddl: false,
  estimated_time_ms: 1,
  status: 'todo' as string,
  priority: 3,
  parent_id: undefined as string | undefined,
})

const loadData = async () => {
  try {
    // 确保默认项目存在
    await getOrCreateDefaultProject()

    // 加载所有任务
    const response = await tasksApi.getAll()
    tasks.value = response.data

    // 检查并同步currentTimeSlice与实际working状态的任务
    const workingTask = tasks.value.find(t => t.execution_state === 'working')

    if (workingTask) {
      // 如果有任务在工作，从数据库中找到正在进行的时间片（end_at为null）
      const ongoingSlice = workingTask.time_slices.find(slice => slice.end_at === null)

      if (ongoingSlice) {
        // 找到了正在进行的时间片，恢复它
        if (!currentTimeSlice.value) {
          currentTimeSlice.value = {
            id: ongoingSlice.id,
            start_at: ongoingSlice.start_at,
            task_id: workingTask.id,
          }
          message.warning(`检测到任务「${workingTask.title}」正在工作中，已自动恢复计时`)
        } else if (currentTimeSlice.value.task_id !== workingTask.id) {
          // currentTimeSlice指向的任务与实际working的任务不一致，需要修正
          currentTimeSlice.value = {
            id: ongoingSlice.id,
            start_at: ongoingSlice.start_at,
            task_id: workingTask.id,
          }
          message.warning(`检测到任务「${workingTask.title}」正在工作中，已同步时间片记录`)
        }
      } else {
        // 理论上不应该出现这种情况（working状态但没有进行中的时间片）
        // 这种情况说明数据不一致，需要修正任务状态
        console.warn('检测到数据不一致：任务状态为working但没有进行中的时间片')
        await tasksApi.update(workingTask.id, { execution_state: 'idle' })
        currentTimeSlice.value = null
        message.error(`任务「${workingTask.title}」的状态不一致，已自动修正`)
      }
    } else {
      // 没有任务在工作，清空currentTimeSlice
      if (currentTimeSlice.value) {
        currentTimeSlice.value = null
      }
    }
  } catch (error) {
    message.error('加载数据失败')
  }
}

const getStatusIcon = (status: string) => {
  const icons: Record<string, string> = {
    todo: '⬜',
    in_progress: '⏳',
    suspended: '⏸️',
    completed: '✅',
  }
  return icons[status] || '•'
}

// 构建任务树（扁平化，不显示项目层级）
const taskTreeData = computed((): TreeOption[] => {
  // 过滤任务（根据搜索关键词）
  const filteredTasks = searchKeyword.value
    ? tasks.value.filter(t =>
        t.title.toLowerCase().includes(searchKeyword.value.toLowerCase()) ||
        (t.description && t.description.toLowerCase().includes(searchKeyword.value.toLowerCase()))
      )
    : tasks.value

  // 构建任务标签（带工作状态高亮）
  const buildLabel = (task: Task): string => {
    const statusIcon = getStatusIcon(task.status)
    const isWorking = task.execution_state === 'working'

    if (isWorking) {
      // 正在工作的任务：添加工作图标
      return `${statusIcon} ⏱️ ${task.title}`
    }

    return `${statusIcon} ${task.title}`
  }

  // 如果有搜索关键词，显示所有匹配的任务（扁平化）
  if (searchKeyword.value) {
    return filteredTasks.map(task => ({
      key: task.id,
      label: buildLabel(task),
      task: task,
      prefix: task.execution_state === 'working' ? () => '🔴' : undefined,
    }))
  }

  // 没有搜索时，显示树形结构
  const topLevelTasks = filteredTasks
    .filter(t => !t.parent_id)
    .sort((a, b) => a.position - b.position)

  const buildTree = (task: Task): TreeOption => {
    const children = filteredTasks
      .filter(t => t.parent_id === task.id)
      .sort((a, b) => a.position - b.position)
    return {
      key: task.id,
      label: buildLabel(task),
      task: task,
      prefix: task.execution_state === 'working' ? () => '🔴' : undefined,
      children: children.length > 0 ? children.map(buildTree) : undefined,
    }
  }

  return topLevelTasks.map(buildTree)
})

// 用于父任务选择的树形数据（排除当前任务及其后代，避免循环引用）
const parentTaskTreeData = computed(() => {
  const excludeTaskId = editTask.value?.id // 编辑时排除自己

  // 获取某任务的所有后代ID
  const getAllDescendantIds = (taskId: string): Set<string> => {
    const descendants = new Set<string>()
    const addDescendants = (id: string) => {
      const children = tasks.value.filter(t => t.parent_id === id)
      children.forEach(child => {
        descendants.add(child.id)
        addDescendants(child.id)
      })
    }
    addDescendants(taskId)
    return descendants
  }

  // 获取要排除的任务ID集合
  const excludedIds = excludeTaskId ? new Set([excludeTaskId, ...getAllDescendantIds(excludeTaskId)]) : new Set()

  // 过滤并构建树
  const availableTasks = tasks.value.filter(t => !excludedIds.has(t.id))
  const topLevelTasks = availableTasks.filter(t => !t.parent_id)

  const buildTree = (task: Task): TreeOption => {
    const children = availableTasks.filter(t => t.parent_id === task.id)
    return {
      key: task.id,
      label: task.title,
      children: children.length > 0 ? children.map(buildTree) : undefined,
    }
  }

  return topLevelTasks.map(buildTree)
})

const statusOptions = [
  { label: '⬜ 待办', value: 'todo' },
  { label: '⏳ 进行中', value: 'in_progress' },
  { label: '⏸️ 挂起', value: 'suspended' },
  { label: '✅ 完成', value: 'completed' },
]

const getStatusColor = (status: TaskStatus): string => {
  const colors: Record<TaskStatus, string> = {
    todo: '#9ca3af',          // 灰色
    in_progress: '#60a5fa',   // 浅蓝色
    suspended: '#fbbf24',     // 浅黄色
    completed: '#34d399',     // 浅绿色
  }
  return colors[status] || '#9ca3af'
}

const getStatusLabel = (status: TaskStatus): string => {
  const labels: Record<TaskStatus, string> = {
    todo: '待办',
    in_progress: '进行中',
    suspended: '挂起',
    completed: '完成',
  }
  return labels[status] || status
}

const getExecutionLabel = (execution_state: string): string => {
  return execution_state === 'working' ? '工作中' : '空闲'
}

// 获取有效的截止时间（考虑父任务继承）
const getEffectiveDueAt = (task: Task): string | null => {
  // 如果任务自己有截止时间，直接返回
  if (task.due_at) {
    return task.due_at
  }

  // 如果没有，向上查找父任务的截止时间
  if (task.parent_id) {
    const parent = tasks.value.find(t => t.id === task.parent_id)
    if (parent) {
      return getEffectiveDueAt(parent)
    }
  }

  return null
}

// 获取是否启用DDL（考虑父任务继承）
const getEffectiveIsDDL = (task: Task): boolean => {
  // 如果任务自己启用了DDL，返回true
  if (task.is_ddl) {
    return true
  }

  // 如果没有，向上查找父任务是否启用DDL
  if (task.parent_id) {
    const parent = tasks.value.find(t => t.id === task.parent_id)
    if (parent) {
      return getEffectiveIsDDL(parent)
    }
  }

  return false
}

// 检查任务是否已锁定（DDL已过期）
const isTaskLocked = (task: Task): boolean => {
  const effectiveDueAt = getEffectiveDueAt(task)
  const effectiveIsDDL = getEffectiveIsDDL(task)

  if (!effectiveDueAt || !effectiveIsDDL) {
    return false
  }

  const dueDate = new Date(effectiveDueAt)
  const now = new Date()
  return now > dueDate
}

// 检查任务是否超时（但未必锁定）
const isTaskOverdue = (task: Task): boolean => {
  const effectiveDueAt = getEffectiveDueAt(task)
  if (!effectiveDueAt) {
    return false
  }

  const dueDate = new Date(effectiveDueAt)
  const now = new Date()
  return now > dueDate
}

// 守卫函数：确保任务可编辑（统一入口）
const ensureTaskEditable = (task: Task, action: string = '操作'): boolean => {
  if (isTaskLocked(task)) {
    message.error(`🔒 任务已锁定，无法${action}`)
    return false
  }
  return true
}

// 获取有效的预期时间（考虑子任务）
const getEffectiveEstimatedTime = (task: Task): number | null => {
  // 如果任务有自己的预期时间，直接返回
  if (task.estimated_time_ms !== null) {
    return task.estimated_time_ms
  }

  // 如果任务没有预期时间，递归计算所有子任务的预期时间总和
  const children = tasks.value.filter(t => t.parent_id === task.id)
  if (children.length === 0) {
    return null // 没有子任务，返回 null
  }

  let total = 0
  let hasAnyEstimate = false

  for (const child of children) {
    const childEstimate = getEffectiveEstimatedTime(child)
    if (childEstimate !== null) {
      total += childEstimate
      hasAnyEstimate = true
    }
  }

  return hasAnyEstimate ? total : null
}

// 计算时间效率（预期时间/实际时间，越大越好）
const calculateTimeEfficiency = (task: Task): number => {
  // 获取有效的预期时间
  const effectiveEstimatedTime = getEffectiveEstimatedTime(task)
  if (effectiveEstimatedTime === null) return 0

  // 计算实际用时 = 自己的用时 + 所有子任务的用时
  const totalActualTime = task.total_logged_ms + calculateChildrenTotalTime(task.id)

  if (totalActualTime === 0) return 0
  return (effectiveEstimatedTime / totalActualTime) * 100
}

// 计算工作效率（时间片效率评分的加权平均，越大越好）
// 递归计算任务及其所有子任务的加权效率
const calculateWorkEfficiency = (task: Task): number => {
  let totalWeightedScore = 0
  let totalDuration = 0

  // 计算自己的时间片
  for (const slice of task.time_slices) {
    if (slice.duration_ms !== null && slice.efficiency_score !== null) {
      totalWeightedScore += slice.duration_ms * slice.efficiency_score
      totalDuration += slice.duration_ms
    }
  }

  // 递归计算所有子任务的效率
  const children = tasks.value.filter(t => t.parent_id === task.id)
  for (const child of children) {
    const childEfficiency = calculateWorkEfficiency(child)
    const childDuration = child.total_logged_ms + calculateChildrenTotalTime(child.id)
    if (childDuration > 0) {
      totalWeightedScore += childDuration * childEfficiency
      totalDuration += childDuration
    }
  }

  if (totalDuration === 0) return 0
  return totalWeightedScore / totalDuration
}

// 计算子任务的总用时（递归统计所有后代）
const calculateChildrenTotalTime = (taskId: string): number => {
  const children = tasks.value.filter(t => t.parent_id === taskId)
  let total = 0

  for (const child of children) {
    // 子任务自己的用时
    total += child.total_logged_ms
    // 递归计算子任务的子任务
    total += calculateChildrenTotalTime(child.id)
  }

  return total
}

// 判断任务是否有子任务
const hasChildren = (taskId: string): boolean => {
  return tasks.value.some(t => t.parent_id === taskId)
}

// 精确的时间格式化函数（支持天、小时、分钟、秒）
const formatDuration = (ms: number): string => {
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

const priorityOptions = [
  { label: 'P1 - 最高', value: 1 },
  { label: 'P2 - 高', value: 2 },
  { label: 'P3 - 中', value: 3 },
  { label: 'P4 - 低', value: 4 },
  { label: 'P5 - 最低', value: 5 },
]

const handleSelect = (keys: Array<string | number>, option: Array<TreeOption | null>) => {
  const selected = option[0]
  if (selected && 'task' in selected) {
    selectedTask.value = selected.task as Task
    // 切换任务时清空选中的时间片
    selectedTimeSlice.value = null
  }
}

const handleCreateTask = async () => {
  if (!newTask.value.title) {
    message.warning('请输入任务标题')
    return
  }

  try {
    // 获取默认项目ID
    const defaultProject = await getOrCreateDefaultProject()

    const taskData: TaskCreate = {
      project_id: defaultProject.id,
      title: newTask.value.title,
      description: newTask.value.description,
      due_at: newTask.value.due_at ? new Date(newTask.value.due_at).toISOString() : undefined,
      is_ddl: newTask.value.is_ddl,
      estimated_time_ms: newTask.value.estimated_time_ms != null ? newTask.value.estimated_time_ms * 3600000 : null, // 转换为毫秒
      parent_id: newTask.value.parent_id,
    }

    await tasksApi.create(taskData)
    message.success('任务创建成功')
    showCreateModal.value = false

    // 重置表单
    newTask.value = {
      title: '',
      description: '',
      estimated_time_ms: 1,
      parent_id: undefined,
    }

    loadData()
  } catch (error) {
    message.error('创建失败')
    console.error(error)
  }
}

const handleEditTask = (task: Task) => {
  // 守卫：检查任务是否可编辑
  if (!ensureTaskEditable(task, '编辑')) return

  // 格式化截止时间
  const formatDateTime = (isoString: string | undefined) => {
    if (!isoString) return null
    const date = new Date(isoString)
    const year = date.getFullYear()
    const month = String(date.getMonth() + 1).padStart(2, '0')
    const day = String(date.getDate()).padStart(2, '0')
    const hours = String(date.getHours()).padStart(2, '0')
    const minutes = String(date.getMinutes()).padStart(2, '0')
    const seconds = String(date.getSeconds()).padStart(2, '0')
    return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`
  }

  editTask.value = {
    id: task.id,
    title: task.title,
    description: task.description || '',
    due_at: formatDateTime(task.due_at),
    is_ddl: task.is_ddl,
    estimated_time_ms: task.estimated_time_ms != null ? task.estimated_time_ms / 3600000 : null, // 转换为小时
    status: task.status,
    priority: task.priority,
    parent_id: task.parent_id,
  }
  showEditModal.value = true
}

const handleUpdateTask = async () => {
  if (!editTask.value.title) {
    message.warning('请输入任务标题')
    return
  }

  try {
    await tasksApi.update(editTask.value.id, {
      title: editTask.value.title,
      description: editTask.value.description,
      due_at: editTask.value.due_at ? new Date(editTask.value.due_at).toISOString() : null,
      is_ddl: editTask.value.is_ddl,
      estimated_time_ms: editTask.value.estimated_time_ms != null ? editTask.value.estimated_time_ms * 3600000 : null,
      status: editTask.value.status as any,
      priority: editTask.value.priority,
      parent_id: editTask.value.parent_id,
    })
    message.success('任务更新成功')
    showEditModal.value = false
    loadData()

    // 更新选中的任务
    if (selectedTask.value && selectedTask.value.id === editTask.value.id) {
      const response = await tasksApi.get(editTask.value.id)
      selectedTask.value = response.data
    }
  } catch (error) {
    message.error('更新失败')
    console.error(error)
  }
}

// 计算任务及其所有后代的数量
const countTaskAndDescendants = (taskId: string): number => {
  const children = tasks.value.filter(t => t.parent_id === taskId)
  let count = 1 // 自己

  for (const child of children) {
    count += countTaskAndDescendants(child.id)
  }

  return count
}

const handleDeleteTask = async (taskId: string) => {
  // 守卫：检查任务是否可编辑
  const task = tasks.value.find(t => t.id === taskId)
  if (task && !ensureTaskEditable(task, '删除')) return

  // 检查是否有子任务
  const childrenCount = tasks.value.filter(t => t.parent_id === taskId).length

  if (childrenCount > 0) {
    const totalCount = countTaskAndDescendants(taskId)

    // 使用dialog确认
    dialog.warning({
      title: '⚠️ 确认删除',
      content: `该任务包含 ${childrenCount} 个直接子任务，共 ${totalCount} 个任务（包括所有后代）将被删除。此操作不可恢复！`,
      positiveText: '确定删除',
      negativeText: '取消',
      onPositiveClick: async () => {
        try {
          await tasksApi.delete(taskId)
          message.success(`已删除 ${totalCount} 个任务`)

          // 如果删除的是当前选中的任务，清空选择
          if (selectedTask.value && selectedTask.value.id === taskId) {
            selectedTask.value = null
          }

          loadData()
        } catch (error) {
          message.error('删除失败')
          console.error(error)
        }
      },
    })
  } else {
    // 没有子任务，直接删除
    try {
      await tasksApi.delete(taskId)
      message.success('任务已删除')

      // 如果删除的是当前选中的任务，清空选择
      if (selectedTask.value && selectedTask.value.id === taskId) {
        selectedTask.value = null
      }

      loadData()
    } catch (error) {
      message.error('删除失败')
      console.error(error)
    }
  }
}

// 处理解锁任务
const handleUnlockTask = async () => {
  if (!selectedTask.value) return

  dialog.warning({
    title: '⚠️ 确认解锁',
    content: '解锁后，该任务的DDL模式将被禁用，即使超过截止时间也可以继续编辑。此操作不会影响父任务或子任务的DDL设置。确定要解锁吗？',
    positiveText: '确定解锁',
    negativeText: '取消',
    onPositiveClick: async () => {
      try {
        await tasksApi.unlock(selectedTask.value!.id)
        message.success('🔓 任务已解锁')

        // 重新加载任务数据
        await loadData()

        // 刷新当前选中的任务
        if (selectedTask.value) {
          const response = await tasksApi.get(selectedTask.value.id)
          selectedTask.value = response.data
        }
      } catch (error) {
        message.error('解锁失败')
        console.error(error)
      }
    },
  })
}

// 处理热力图日期点击
const handleHeatmapDateClick = (dateStr: string) => {
  if (dailyChartRef.value) {
    dailyChartRef.value.goToDate(dateStr)
    message.success(`已跳转到 ${dateStr}`)
  }
}

// 处理日表时间片点击
const handleDailyChartTaskClick = (taskId: string) => {
  const task = tasks.value.find(t => t.id === taskId)
  if (task) {
    selectedTask.value = task
    message.success(`已选中任务：${task.title}`)
  }
}

// 处理逻辑状态切换（生命周期状态）
const handleStatusChange = async (newStatus: TaskStatus) => {
  if (!selectedTask.value) return

  const oldStatus = selectedTask.value.status
  const task = selectedTask.value

  console.log('逻辑状态切换:', oldStatus, '->', newStatus)

  // 状态相同，不处理
  if (oldStatus === newStatus) {
    return
  }

  // 如果任务有时间片记录，不能切换回 todo
  if (newStatus === 'todo' && task.time_slices.length > 0) {
    message.warning('该任务已有时间片记录，不能切换回待办状态')
    return
  }

  // 如果正在工作中，不能直接切换状态
  if (task.execution_state === 'working') {
    message.warning('请先停止工作再切换状态')
    return
  }

  try {
    // 切换到非进行中状态时，自动将执行状态设为 idle
    const updates: any = { status: newStatus }
    if (newStatus !== 'in_progress') {
      updates.execution_state = 'idle'
    }

    await tasksApi.update(task.id, updates)
    message.success('状态已更新')

    // 重新加载数据
    await loadData()
    const response = await tasksApi.get(task.id)
    selectedTask.value = response.data
  } catch (error) {
    message.error('状态更新失败')
    console.error(error)
  }
}

// 开始工作
const handleStartWork = async () => {
  if (!selectedTask.value) return

  const task = selectedTask.value

  // 守卫：检查任务是否可编辑
  if (!ensureTaskEditable(task, '开始工作')) return

  // 只有进行中状态才能开始工作
  if (task.status !== 'in_progress') {
    message.warning('请先将任务状态改为"进行中"')
    return
  }

  // 检查是否有其他任务正在工作（互斥逻辑）
  const workingTask = tasks.value.find(t => t.id !== task.id && t.execution_state === 'working')
  if (workingTask) {
    message.warning(`任务「${workingTask.title}」正在工作中，请先停止该任务再开始新任务`)
    return
  }

  try {
    const startTime = new Date().toISOString()

    // 立即创建数据库记录（不传 end_at/efficiency_score/note，使用默认值）
    const response = await timeSlicesApi.create({
      task_id: task.id,
      start_at: startTime,
    } as TimeSliceCreate)

    // 保存当前时间片（包含 ID）
    currentTimeSlice.value = {
      id: response.data.id,
      start_at: response.data.start_at,
      task_id: task.id,
    }

    // 更新执行状态为 working
    await tasksApi.update(task.id, { execution_state: 'working' })
    message.success('开始工作计时...')

    // 重新加载数据
    await loadData()
    const taskResponse = await tasksApi.get(task.id)
    selectedTask.value = taskResponse.data
  } catch (error) {
    message.error('开始工作失败')
    console.error(error)
  }
}

// 停止工作
const handleStopWork = async () => {
  if (!selectedTask.value || !currentTimeSlice.value) return

  // 显示时间片输入弹窗
  showTimeSliceModal.value = true
}

const handleTimeSliceSubmit = async () => {
  if (!currentTimeSlice.value || !selectedTask.value) return

  try {
    const endTime = new Date()
    const startTime = new Date(currentTimeSlice.value.start_at)
    const durationMs = endTime.getTime() - startTime.getTime()

    // 更新已有时间片记录（不是创建新记录）
    await timeSlicesApi.update(currentTimeSlice.value.id, {
      end_at: endTime.toISOString(),
      duration_ms: durationMs,
      efficiency_score: timeSliceForm.value.efficiency_score,
      note: timeSliceForm.value.note,
    })

    // 更新执行状态为 idle
    await tasksApi.update(selectedTask.value.id, { execution_state: 'idle' })

    message.success(`时间片已保存 (${formatDuration(durationMs)})`)
    showTimeSliceModal.value = false

    // 重置表单
    timeSliceForm.value = {
      efficiency_score: 3,
      note: '',
    }
    currentTimeSlice.value = null

    // 重新加载数据
    await loadData()

    // 更新选中的任务
    const response = await tasksApi.get(selectedTask.value.id)
    selectedTask.value = response.data
  } catch (error) {
    message.error('保存时间片失败')
    console.error('保存时间片错误:', error)
  }
}

const handleTimeSliceCancel = () => {
  // 取消停止动作，继续工作
  showTimeSliceModal.value = false
  // 重置表单，但保持 currentTimeSlice 不变，以便继续工作
  timeSliceForm.value = {
    efficiency_score: 3,
    note: '',
  }
  // 不清空 currentTimeSlice，保持工作状态
  message.info('取消停止，继续工作中...')
}

// 处理时间片点击
const handleTimeSliceClick = (slice: TimeSlice) => {
  selectedTimeSlice.value = slice
}

// 打开编辑时间片弹窗
const handleEditTimeSlice = () => {
  if (!selectedTimeSlice.value) return

  // 转换 ISO 格式到 yyyy-MM-dd HH:mm:ss 格式
  const formatDateTime = (isoString: string) => {
    const date = new Date(isoString)
    const year = date.getFullYear()
    const month = String(date.getMonth() + 1).padStart(2, '0')
    const day = String(date.getDate()).padStart(2, '0')
    const hours = String(date.getHours()).padStart(2, '0')
    const minutes = String(date.getMinutes()).padStart(2, '0')
    const seconds = String(date.getSeconds()).padStart(2, '0')
    return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`
  }

  editTimeSliceForm.value = {
    id: selectedTimeSlice.value.id,
    start_at: formatDateTime(selectedTimeSlice.value.start_at),
    end_at: selectedTimeSlice.value.end_at ? formatDateTime(selectedTimeSlice.value.end_at) : '',
    efficiency_score: selectedTimeSlice.value.efficiency_score || 3,
    note: selectedTimeSlice.value.note || '',
  }
  showEditTimeSliceModal.value = true
}

// 保存编辑后的时间片
const handleUpdateTimeSlice = async () => {
  if (!editTimeSliceForm.value.id) return

  try {
    const startTime = new Date(editTimeSliceForm.value.start_at)
    const endTime = new Date(editTimeSliceForm.value.end_at)
    const durationMs = endTime.getTime() - startTime.getTime()

    if (durationMs <= 0) {
      message.warning('结束时间必须晚于开始时间')
      return
    }

    // 转换为 ISO 格式
    await timeSlicesApi.update(editTimeSliceForm.value.id, {
      start_at: startTime.toISOString(),
      end_at: endTime.toISOString(),
      duration_ms: durationMs,
      efficiency_score: editTimeSliceForm.value.efficiency_score,
      note: editTimeSliceForm.value.note || null,
    })

    message.success('时间片更新成功')
    showEditTimeSliceModal.value = false

    // 重新加载数据
    await loadData()

    // 更新选中的时间片
    if (selectedTask.value) {
      const response = await tasksApi.get(selectedTask.value.id)
      selectedTask.value = response.data
      // 更新选中的时间片引用
      selectedTimeSlice.value = response.data.time_slices.find(
        (ts: TimeSlice) => ts.id === editTimeSliceForm.value.id
      ) || null
    }
  } catch (error) {
    message.error('更新时间片失败')
    console.error('更新时间片错误:', error)
  }
}

// 删除时间片（带确认）
const handleDeleteTimeSlice = () => {
  if (!selectedTimeSlice.value) return

  dialog.warning({
    title: '确认删除',
    content: `确定要删除这条时间片记录吗？\n\n开始时间：${new Date(selectedTimeSlice.value.start_at).toLocaleString('zh-CN')}\n工作时长：${formatDuration(selectedTimeSlice.value.duration_ms || 0)}\n\n此操作不可恢复！`,
    positiveText: '确认删除',
    negativeText: '取消',
    onPositiveClick: async () => {
      try {
        await timeSlicesApi.delete(selectedTimeSlice.value!.id)
        message.success('时间片已删除')
        selectedTimeSlice.value = null

        // 重新加载数据
        await loadData()

        // 更新选中的任务
        if (selectedTask.value) {
          const response = await tasksApi.get(selectedTask.value.id)
          selectedTask.value = response.data
        }
      } catch (error) {
        message.error('删除时间片失败')
        console.error('删除时间片错误:', error)
      }
    },
  })
}

// 处理从回收站恢复任务
const handleTaskRestored = async () => {
  // 重新加载任务列表
  await loadData()
  message.success('任务已从回收站恢复')
}

// 为NTree节点添加自定义属性（用于高亮正在工作的任务、右键菜单和优先级颜色）
const getNodeProps = ({ option }: { option: TreeOption }) => {
  const task = option.task as Task | undefined
  const props: any = {}

  // 添加右键事件处理
  if (task) {
    props.onContextmenu = (e: MouseEvent) => {
      handleNodeContextMenu(e, task)
    }
  }

  // 添加优先级样式类
  if (task) {
    const classes = []

    // 工作中任务的高亮样式
    if (task.execution_state === 'working') {
      classes.push('working-task-node')
    }

    // 优先级颜色样式
    classes.push(`priority-${task.priority}`)

    if (classes.length > 0) {
      props.class = classes.join(' ')
    }
  }

  // 添加原生拖拽事件监听
  props.onDragStart = (e: DragEvent) => {
    draggingNode.value = { option }
  }

  props.onDragEnd = (e: DragEvent) => {
    draggingNode.value = null
  }

  return props
}

// 拖拽开始：记录被拖拽的节点
const handleDragStart = (info: any) => {
  draggingNode.value = info.node
}

// 拖拽结束：清除记录
const handleDragEnd = (info: any) => {
  draggingNode.value = null
}

// 验证拖拽：仅允许同级任务之间拖拽
const allowDrop = (info: any) => {
  const { dropPosition, node, dragNode } = info
  const effectiveDragNode = dragNode || draggingNode.value

  if (!effectiveDragNode) {
    return false
  }

  if (searchKeyword.value) {
    return false
  }

  const dropTask = node?.option?.task || node?.task || node?.rawNode?.task
  const dragTask = effectiveDragNode?.option?.task || effectiveDragNode?.task || effectiveDragNode?.rawNode?.task

  if (!dropTask || !dragTask) {
    return false
  }

  if (dropPosition === 'inside') {
    return false
  }

  return dropTask.parent_id === dragTask.parent_id
}

// 处理拖拽放置：计算新位置并更新
const handleDrop = async (info: any) => {
  const { node, dragNode, dropPosition } = info
  const effectiveDragNode = dragNode || draggingNode.value

  const dragTask = effectiveDragNode?.task || effectiveDragNode?.option?.task || effectiveDragNode?.rawNode?.task
  const dropTask = node?.task || node?.option?.task || node?.rawNode?.task

  if (!dragTask || !dropTask) {
    message.error('拖拽失败：任务数据不存在')
    return
  }

  // 守卫：检查任务是否可拖拽
  if (!ensureTaskEditable(dragTask, '移动')) return

  // 验证：如果正在搜索，禁止拖拽
  if (searchKeyword.value) {
    message.warning('搜索模式下不允许拖拽任务')
    return
  }

  // 验证：不允许设为子任务
  if (dropPosition === 'inside') {
    message.warning('不允许将任务设为其他任务的子任务，只能调整同级顺序')
    return
  }

  // 验证：只允许同级任务之间拖拽
  if (dropTask.parent_id !== dragTask.parent_id) {
    message.warning('只能在同级任务之间调整顺序')
    return
  }

  // 获取所有同级任务，按position排序
  const siblings = tasks.value
    .filter(t => t.parent_id === dragTask.parent_id)
    .sort((a, b) => a.position - b.position)

  // 计算新的position
  let newPosition: number
  const dropIndex = siblings.findIndex(t => t.id === dropTask.id)

  if (dropPosition === 'before') {
    if (dropIndex === 0) {
      newPosition = dropTask.position / 2
    } else {
      const prevTask = siblings[dropIndex - 1]
      newPosition = (prevTask.position + dropTask.position) / 2
    }
  } else if (dropPosition === 'after') {
    if (dropIndex === siblings.length - 1) {
      newPosition = dropTask.position + 10
    } else {
      const nextTask = siblings[dropIndex + 1]
      newPosition = (dropTask.position + nextTask.position) / 2
    }
  } else {
    return
  }

  if (isNaN(newPosition)) {
    message.error('位置计算错误，请检查任务数据')
    return
  }

  try {
    await tasksApi.updatePosition(dragTask.id, newPosition)
    await loadData()
    message.success('任务顺序已更新')
  } catch (error: any) {
    message.error('更新失败: ' + (error.response?.data?.detail || error.message))
  }
}

const stats = computed(() => {
  const totalTasks = tasks.value.length
  const completedTasks = tasks.value.filter(t => t.status === 'completed').length
  const totalTime = tasks.value.reduce((sum, t) => sum + t.total_logged_ms, 0)
  return {
    totalTasks,
    completedTasks,
    totalTime,
    completionRate: totalTasks > 0 ? (completedTasks / totalTasks) * 100 : 0,
  }
})

// 检查并恢复未完成的时间片
const checkUnfinishedTimeSlices = () => {
  // 查找所有 end_at 为 null 的时间片
  const unfinishedSlices = tasks.value
    .flatMap(t => t.time_slices)
    .filter(ts => ts.end_at === null)

  if (unfinishedSlices.length > 0) {
    const slice = unfinishedSlices[0]
    const task = tasks.value.find(t => t.id === slice.task_id)

    if (task) {
      // 恢复工作状态
      currentTimeSlice.value = {
        id: slice.id,
        start_at: slice.start_at,
        task_id: slice.task_id,
      }

      // 自动选中正在工作的任务
      selectedTask.value = task

      const startTime = new Date(slice.start_at)
      const now = new Date()
      const elapsedMinutes = Math.floor((now.getTime() - startTime.getTime()) / (1000 * 60))

      message.info(`已恢复任务「${task.title}」的工作状态（已进行 ${elapsedMinutes} 分钟）`)
    }
  }
}

// 检查并自动停止DDL过期的工作
const checkDDLExpiry = async () => {
  if (!currentTimeSlice.value || !selectedTask.value) return

  const locked = isTaskLocked(selectedTask.value)
  if (locked) {
    // 任务DDL已过期，自动停止工作
    try {
      const endTime = new Date()
      const startTime = new Date(currentTimeSlice.value.start_at)
      const durationMs = endTime.getTime() - startTime.getTime()

      // 保存时间片，备注填写"任务截止"
      await timeSlicesApi.update(currentTimeSlice.value.id, {
        end_at: endTime.toISOString(),
        duration_ms: durationMs,
        efficiency_score: 3, // 默认评分
        note: '任务截止，自动停止',
      })

      // 更新任务状态
      await tasksApi.update(selectedTask.value.id, { execution_state: 'idle' })

      currentTimeSlice.value = null
      message.warning('⏰ 任务DDL已到期，已自动停止工作')

      // 重新加载数据
      await loadData()
      if (selectedTask.value) {
        const response = await tasksApi.get(selectedTask.value.id)
        selectedTask.value = response.data
      }
    } catch (error) {
      console.error('自动停止工作失败:', error)
    }
  }
}

// DDL检查定时器
let ddlCheckInterval: ReturnType<typeof setInterval> | null = null

onMounted(async () => {
  loadExpandedKeys()
  await loadData()
  checkUnfinishedTimeSlices()

  // 每分钟检查一次DDL过期
  ddlCheckInterval = setInterval(() => {
    checkDDLExpiry()
  }, 60000) // 60秒
})

onBeforeUnmount(() => {
  // 组件卸载时清除定时器
  if (ddlCheckInterval) {
    clearInterval(ddlCheckInterval)
  }
})
</script>

<style scoped>
.time-slice-item {
  padding: 12px 16px;
  border-bottom: 1px solid #f0f0f0;
  cursor: pointer;
  background: transparent;
  transition: background 0.2s;
}

.time-slice-item:hover {
  background: #f9fafb;
}

.time-slice-item.selected {
  background: #e0f2fe !important;
}

/* 正在工作的任务高亮样式 */
:deep(.working-task-node) {
  font-weight: 600 !important;
  color: #dc2626 !important;
  border-left: 3px solid #dc2626 !important;
  padding-left: 5px !important;
  animation: pulse 2s ease-in-out infinite;
}

/* 优先级背景色样式 */
:deep(.priority-1) {
  background: linear-gradient(90deg, rgba(147, 51, 234, 0.25) 0%, rgba(147, 51, 234, 0.12) 100%) !important;
}

:deep(.priority-2) {
  background: linear-gradient(90deg, rgba(239, 68, 68, 0.25) 0%, rgba(239, 68, 68, 0.12) 100%) !important;
}

:deep(.priority-3) {
  background: linear-gradient(90deg, rgba(234, 179, 8, 0.2) 0%, rgba(234, 179, 8, 0.08) 100%) !important;
}

:deep(.priority-4) {
  background: linear-gradient(90deg, rgba(59, 130, 246, 0.2) 0%, rgba(59, 130, 246, 0.08) 100%) !important;
}

:deep(.priority-5) {
  background: linear-gradient(90deg, rgba(156, 163, 175, 0.2) 0%, rgba(156, 163, 175, 0.08) 100%) !important;
}

/* 工作中任务叠加优先级样式 */
:deep(.working-task-node.priority-1) {
  background: linear-gradient(90deg, rgba(147, 51, 234, 0.3) 0%, rgba(147, 51, 234, 0.15) 100%) !important;
}

:deep(.working-task-node.priority-2) {
  background: linear-gradient(90deg, rgba(239, 68, 68, 0.3) 0%, rgba(239, 68, 68, 0.15) 100%) !important;
}

:deep(.working-task-node.priority-3) {
  background: linear-gradient(90deg, rgba(234, 179, 8, 0.25) 0%, rgba(234, 179, 8, 0.12) 100%) !important;
}

:deep(.working-task-node.priority-4) {
  background: linear-gradient(90deg, rgba(59, 130, 246, 0.25) 0%, rgba(59, 130, 246, 0.12) 100%) !important;
}

:deep(.working-task-node.priority-5) {
  background: linear-gradient(90deg, rgba(156, 163, 175, 0.25) 0%, rgba(156, 163, 175, 0.12) 100%) !important;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.85;
  }
}
</style>

<template>
  <NLayout style="height: 100vh">
    <NLayoutHeader bordered style="height: 64px; padding: 0 24px; display: flex; align-items: center; justify-content: space-between">
      <h1 style="margin: 0; font-size: 20px">⏱️ TBox</h1>
      <div style="display: flex; gap: 12px">
        <NButton @click="showTrash = true">
          🗑️ 回收站
        </NButton>
        <NButton type="primary" @click="showCreateModal = true">+ 创建任务</NButton>
      </div>
    </NLayoutHeader>

    <NLayoutContent style="height: calc(100vh - 64px)">
      <div style="display: grid; grid-template-columns: 25% 50% 25%; height: 100%; gap: 1px; background: #e5e7eb">

        <div style="background: white; padding: 16px; overflow: auto">
          <h3 style="margin-bottom: 12px">任务列表</h3>
          <NInput
            v-model:value="searchKeyword"
            placeholder="搜索任务..."
            clearable
            style="margin-bottom: 12px"
          >
            <template #prefix>🔍</template>
          </NInput>
          <NTree
            v-if="taskTreeData.length > 0"
            :data="taskTreeData"
            block-line
            selectable
            draggable
            :node-props="getNodeProps"
            :expanded-keys="expandedKeys"
            @update:expanded-keys="handleExpandedKeysChange"
            @update:selected-keys="handleSelect"
            @drop="handleDrop"
          />
          <div v-else style="text-align: center; color: #999; margin-top: 50px">
            <p v-if="searchKeyword">未找到匹配的任务</p>
            <template v-else>
              <p>暂无任务</p>
              <p style="font-size: 12px; margin-top: 8px">点击右上角创建任务</p>
            </template>
          </div>
        </div>

        <!-- 中间：任务详情 -->
        <div style="background: white; padding: 16px; overflow: auto">
              <div v-if="selectedTask">
                <div style="display: flex; justify-content: space-between; align-items: center">
                  <div style="display: flex; align-items: center; gap: 12px">
                    <h2 style="margin: 0">{{ selectedTask.title }}</h2>
                    <!-- 锁定标记 -->
                    <span v-if="isTaskLocked(selectedTask)" style="padding: 2px 8px; background: #ef4444; color: white; border-radius: 4px; font-size: 12px; font-weight: 600">
                      🔒 已锁定
                    </span>
                    <!-- 超时标记 -->
                    <span v-else-if="isTaskOverdue(selectedTask) && getEffectiveIsDDL(selectedTask)" style="padding: 2px 8px; background: #f59e0b; color: white; border-radius: 4px; font-size: 12px; font-weight: 600">
                      ⏰ 即将锁定
                    </span>
                    <span v-else-if="isTaskOverdue(selectedTask)" style="padding: 2px 8px; background: #f59e0b; color: white; border-radius: 4px; font-size: 12px; font-weight: 600">
                      ⏰ 已超时
                    </span>
                  </div>
                  <NSpace>
                    <NButton size="small" @click="handleEditTask(selectedTask)" :disabled="isTaskLocked(selectedTask)">✏️ 编辑</NButton>
                    <NPopconfirm
                      @positive-click="handleDeleteTask(selectedTask.id)"
                      positive-text="确定"
                      negative-text="取消"
                      :disabled="isTaskLocked(selectedTask)"
                    >
                      <template #trigger>
                        <NButton size="small" type="error" :disabled="isTaskLocked(selectedTask)">🗑️ 删除</NButton>
                      </template>
                      确定要删除此任务吗？
                    </NPopconfirm>
                    <NButton
                      v-if="isTaskLocked(selectedTask)"
                      size="small"
                      type="warning"
                      @click="handleUnlockTask"
                    >
                      🔓 解锁
                    </NButton>
                  </NSpace>
                </div>
                <p style="color: #666; margin-top: 8px">{{ selectedTask.description || '无描述' }}</p>

                <!-- 截止时间显示 -->
                <div v-if="getEffectiveDueAt(selectedTask)" style="margin-top: 8px; padding: 8px; background: #f9fafb; border-radius: 4px; font-size: 13px">
                  <span style="color: #666">📅 截止时间：</span>
                  <span :style="{ color: isTaskOverdue(selectedTask) ? '#ef4444' : '#333', fontWeight: isTaskOverdue(selectedTask) ? '600' : 'normal' }">
                    {{ new Date(getEffectiveDueAt(selectedTask)!).toLocaleString('zh-CN') }}
                  </span>
                  <span v-if="getEffectiveIsDDL(selectedTask)" style="margin-left: 8px; color: #f59e0b">
                    （DDL模式）
                  </span>
                </div>

                <div style="margin-top: 16px">
                  <div style="font-size: 12px; color: #999; margin-bottom: 4px">逻辑状态</div>
                  <div style="display: flex; align-items: center; gap: 12px">
                    <span
                      :style="{
                        display: 'inline-block',
                        padding: '4px 12px',
                        borderRadius: '4px',
                        backgroundColor: getStatusColor(selectedTask.status),
                        color: 'white',
                        fontSize: '14px',
                        fontWeight: '500',
                      }"
                    >
                      {{ getStatusIcon(selectedTask.status) }} {{ getStatusLabel(selectedTask.status) }}
                    </span>
                    <NSelect
                      :value="selectedTask.status"
                      :options="statusOptions"
                      style="width: 160px"
                      @update:value="handleStatusChange"
                    />
                  </div>
                </div>

                <div v-if="selectedTask.status === 'in_progress'" style="margin-top: 16px">
                  <div style="font-size: 12px; color: #999; margin-bottom: 4px">执行状态</div>
                  <div style="display: flex; align-items: center; gap: 12px">
                    <span
                      :style="{
                        display: 'inline-block',
                        padding: '4px 12px',
                        borderRadius: '4px',
                        backgroundColor: selectedTask.execution_state === 'working' ? '#10b981' : '#6b7280',
                        color: 'white',
                        fontSize: '14px',
                        fontWeight: '500',
                      }"
                    >
                      {{ selectedTask.execution_state === 'working' ? '⏱️ 工作中' : '💤 空闲' }}
                    </span>
                    <NButton
                      v-if="selectedTask.execution_state === 'idle'"
                      type="success"
                      size="small"
                      @click="handleStartWork"
                      :disabled="isTaskLocked(selectedTask)"
                    >
                      ▶️ 开始工作
                    </NButton>
                    <NButton
                      v-else
                      type="warning"
                      size="small"
                      @click="handleStopWork"
                    >
                      ⏸️ 停止工作
                    </NButton>
                  </div>
                </div>

                <NGrid :x-gap="12" :y-gap="12" :cols="2" style="margin-top: 24px">
                  <NGridItem>
                    <div>
                      <div style="font-size: 12px; color: #999">优先级</div>
                      <div>P{{ selectedTask.priority }}</div>
                    </div>
                  </NGridItem>
                  <NGridItem>
                    <div>
                      <div style="font-size: 12px; color: #999">预计时间</div>
                      <div>
                        {{
                          getEffectiveEstimatedTime(selectedTask) !== null
                            ? formatDuration(getEffectiveEstimatedTime(selectedTask)!)
                            : '未设定'
                        }}
                        <span v-if="selectedTask.estimated_time_ms === null && getEffectiveEstimatedTime(selectedTask) !== null" style="font-size: 11px; color: #999">
                          (来自子任务)
                        </span>
                      </div>
                    </div>
                  </NGridItem>
                  <NGridItem :span="2">
                    <div>
                      <div style="font-size: 12px; color: #999; margin-bottom: 4px">
                        {{ hasChildren(selectedTask.id) ? '时间统计' : '已用时间' }}
                      </div>
                      <div v-if="!hasChildren(selectedTask.id)">
                        {{ formatDuration(selectedTask.total_logged_ms) }}
                      </div>
                      <div v-else style="display: flex; flex-direction: column; gap: 4px; font-size: 13px">
                        <div style="display: flex; justify-content: space-between">
                          <span style="color: #666">本任务：</span>
                          <span style="font-weight: 500">{{ formatDuration(selectedTask.total_logged_ms) }}</span>
                        </div>
                        <div style="display: flex; justify-content: space-between">
                          <span style="color: #666">子任务：</span>
                          <span style="font-weight: 500">{{ formatDuration(calculateChildrenTotalTime(selectedTask.id)) }}</span>
                        </div>
                        <div style="display: flex; justify-content: space-between; padding-top: 4px; border-top: 1px solid #e5e7eb">
                          <span style="color: #333; font-weight: 600">总计：</span>
                          <span style="color: #18a058; font-weight: 600">{{ formatDuration(selectedTask.total_logged_ms + calculateChildrenTotalTime(selectedTask.id)) }}</span>
                        </div>
                      </div>
                    </div>
                  </NGridItem>
                </NGrid>

                <!-- 效率指标 -->
                <div style="margin-top: 24px">
                  <h4 style="margin-bottom: 12px">📊 效率分析</h4>

                  <!-- 时间效率 -->
                  <div style="margin-bottom: 16px">
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px">
                      <span style="font-size: 12px; color: #999">时间效率（预期÷实际）</span>
                      <span style="font-size: 14px; font-weight: 600" :style="{
                        color: getEffectiveEstimatedTime(selectedTask) === null ? '#9ca3af' :
                               calculateTimeEfficiency(selectedTask) >= 100 ? '#10b981' :
                               calculateTimeEfficiency(selectedTask) >= 80 ? '#f59e0b' : '#ef4444'
                      }">
                        {{
                          getEffectiveEstimatedTime(selectedTask) === null ? '无预期时间' :
                          (selectedTask.total_logged_ms + calculateChildrenTotalTime(selectedTask.id)) > 0 ? calculateTimeEfficiency(selectedTask).toFixed(1) + '%' : '未开始'
                        }}
                      </span>
                    </div>
                    <NProgress
                      v-if="getEffectiveEstimatedTime(selectedTask) !== null && (selectedTask.total_logged_ms + calculateChildrenTotalTime(selectedTask.id)) > 0"
                      type="line"
                      :percentage="Math.min(calculateTimeEfficiency(selectedTask), 200)"
                      :status="calculateTimeEfficiency(selectedTask) >= 100 ? 'success' :
                              calculateTimeEfficiency(selectedTask) >= 80 ? 'warning' : 'error'"
                      :show-indicator="false"
                    />
                    <div v-if="getEffectiveEstimatedTime(selectedTask) !== null && (selectedTask.total_logged_ms + calculateChildrenTotalTime(selectedTask.id)) > 0" style="font-size: 11px; color: #999; margin-top: 4px">
                      {{ calculateTimeEfficiency(selectedTask) >= 100 ? '✨ 提前完成' :
                         calculateTimeEfficiency(selectedTask) >= 80 ? '⚠️ 接近预期' : '🔴 超出预期' }}
                    </div>
                  </div>

                  <!-- 工作效率 -->
                  <div>
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px">
                      <span style="font-size: 12px; color: #999">工作效率（质量评分）</span>
                      <span style="font-size: 14px; font-weight: 600" :style="{
                        color: calculateWorkEfficiency(selectedTask) >= 4 ? '#10b981' :
                               calculateWorkEfficiency(selectedTask) >= 3 ? '#f59e0b' : '#ef4444'
                      }">
                        {{ (selectedTask.total_logged_ms + calculateChildrenTotalTime(selectedTask.id)) > 0 ? calculateWorkEfficiency(selectedTask).toFixed(2) + '/5' : '未评分' }}
                      </span>
                    </div>
                    <NProgress
                      v-if="(selectedTask.total_logged_ms + calculateChildrenTotalTime(selectedTask.id)) > 0"
                      type="line"
                      :percentage="(calculateWorkEfficiency(selectedTask) / 5) * 100"
                      :status="calculateWorkEfficiency(selectedTask) >= 4 ? 'success' :
                              calculateWorkEfficiency(selectedTask) >= 3 ? 'warning' : 'error'"
                      :show-indicator="false"
                    />
                    <div v-if="(selectedTask.total_logged_ms + calculateChildrenTotalTime(selectedTask.id)) > 0" style="font-size: 11px; color: #999; margin-top: 4px">
                      {{ '⭐'.repeat(Math.round(calculateWorkEfficiency(selectedTask))) }}
                    </div>
                  </div>
                </div>

                <!-- 时间片展示区域 -->
                <div style="margin-top: 24px">
                  <h4 style="margin-bottom: 12px">时间片记录 ({{ selectedTask.time_slices.length }})</h4>

                  <div v-if="selectedTask.time_slices.length === 0" style="text-align: center; color: #999; padding: 40px 0; background: #f9f9f9; border-radius: 8px">
                    暂无时间片记录
                  </div>

                  <!-- 左右布局：左侧详情 + 右侧列表 -->
                  <div v-else style="display: flex; gap: 16px; height: 33vh">
                    <!-- 左侧：详情面板 (1份) -->
                    <div style="flex: 1; background: #f9fafb; border-radius: 8px; padding: 16px; overflow-y: auto">
                      <div v-if="selectedTimeSlice">
                        <div style="font-size: 14px; font-weight: 600; color: #333; margin-bottom: 16px">
                          📋 时间片详情
                        </div>

                        <div style="margin-bottom: 12px">
                          <div style="font-size: 12px; color: #999; margin-bottom: 4px">开始时间</div>
                          <div style="font-size: 14px; color: #333">
                            {{ new Date(selectedTimeSlice.start_at).toLocaleString('zh-CN', {
                              year: 'numeric', month: '2-digit', day: '2-digit',
                              hour: '2-digit', minute: '2-digit', second: '2-digit'
                            }) }}
                          </div>
                        </div>

                        <div style="margin-bottom: 12px">
                          <div style="font-size: 12px; color: #999; margin-bottom: 4px">结束时间</div>
                          <div style="font-size: 14px; color: #333">
                            {{ new Date(selectedTimeSlice.end_at).toLocaleString('zh-CN', {
                              year: 'numeric', month: '2-digit', day: '2-digit',
                              hour: '2-digit', minute: '2-digit', second: '2-digit'
                            }) }}
                          </div>
                        </div>

                        <div style="margin-bottom: 12px">
                          <div style="font-size: 12px; color: #999; margin-bottom: 4px">工作时长</div>
                          <div style="font-size: 18px; font-weight: 600; color: #18a058">
                            {{ formatDuration(selectedTimeSlice.duration_ms) }}
                          </div>
                        </div>

                        <div style="margin-bottom: 12px">
                          <div style="font-size: 12px; color: #999; margin-bottom: 4px">效率评分</div>
                          <div style="font-size: 16px; color: #f59e0b">
                            {{ '⭐'.repeat(selectedTimeSlice.efficiency_score) }} ({{ selectedTimeSlice.efficiency_score }}/5)
                          </div>
                        </div>

                        <div v-if="selectedTimeSlice.note" style="margin-bottom: 16px">
                          <div style="font-size: 12px; color: #999; margin-bottom: 4px">备注</div>
                          <div style="font-size: 14px; color: #666; padding: 8px; background: white; border-radius: 4px; word-wrap: break-word">
                            {{ selectedTimeSlice.note }}
                          </div>
                        </div>

                        <!-- 操作按钮 -->
                        <div style="display: flex; gap: 8px; margin-top: 20px">
                          <NButton
                            type="primary"
                            size="small"
                            @click="handleEditTimeSlice"
                            style="flex: 1"
                          >
                            ✏️ 编辑
                          </NButton>
                          <NButton
                            type="error"
                            size="small"
                            @click="handleDeleteTimeSlice"
                            style="flex: 1"
                          >
                            🗑️ 删除
                          </NButton>
                        </div>
                      </div>
                      <div v-else style="text-align: center; color: #999; padding-top: 60px">
                        点击右侧时间片查看详情 →
                      </div>
                    </div>

                    <!-- 右侧：列表面板 (2.5份) -->
                    <div style="flex: 2.5; background: white; border: 1px solid #e5e7eb; border-radius: 8px; overflow-y: auto">
                      <div
                        v-for="slice in selectedTask.time_slices"
                        :key="slice.id"
                        @click="handleTimeSliceClick(slice)"
                        :class="['time-slice-item', { selected: selectedTimeSlice?.id === slice.id }]"
                      >
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px">
                          <div style="font-size: 14px; font-weight: 500; color: #333">
                            {{ new Date(slice.start_at).toLocaleString('zh-CN', {
                              month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit'
                            }) }}
                          </div>
                          <div style="font-size: 14px; font-weight: 600; color: #18a058">
                            {{ formatDuration(slice.duration_ms) }}
                          </div>
                        </div>
                        <div style="display: flex; justify-content: space-between; align-items: center">
                          <div style="font-size: 12px; color: #f59e0b">
                            {{ '⭐'.repeat(slice.efficiency_score) }}
                          </div>
                          <div v-if="slice.note" style="font-size: 12px; color: #999; max-width: 200px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap">
                            {{ slice.note }}
                          </div>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
              <div v-else style="text-align: center; color: #999; margin-top: 100px">
                <p>← 请从左侧选择任务查看详情</p>
              </div>
            </div>

        <!-- 右侧：统计区 -->
        <div style="background: white; display: flex; flex-direction: column; gap: 1px;">
          <!-- 1. 总时间统计条 -->
          <div style="padding: 12px 16px; background: #f9fafb; border-bottom: 1px solid #e5e7eb;">
            <div style="font-size: 20px; font-weight: 600; color: #18a058; margin-bottom: 4px;">
              {{ formatDuration(stats.totalTime) }}
            </div>
            <div style="font-size: 12px; color: #666; display: flex; justify-content: space-between;">
              <span>总任务: {{ stats.totalTasks }}</span>
              <span>已完成: {{ stats.completedTasks }}</span>
            </div>
          </div>

          <!-- 2. 24小时效率图 -->
          <div style="flex: 1; padding: 16px; border-bottom: 1px solid #e5e7eb;">
            <DailyEfficiencyChart
              ref="dailyChartRef"
              :time-slices="tasks.flatMap(t => t.time_slices)"
              @task-click="handleDailyChartTaskClick"
            />
          </div>

          <!-- 3. 本周柱状图 -->
          <div style="flex: 1; padding: 16px; border-bottom: 1px solid #e5e7eb;">
            <WeeklyBarChart
              :time-slices="tasks.flatMap(t => t.time_slices)"
              @date-click="handleHeatmapDateClick"
            />
          </div>

          <!-- 4. 年度活动热力图 -->
          <div style="flex: 1; padding: 16px;">
            <ActivityHeatmap
              :time-slices="tasks.flatMap(t => t.time_slices)"
              @date-click="handleHeatmapDateClick"
            />
          </div>
        </div>
      </div>
    </NLayoutContent>

    <!-- 创建任务弹窗 -->
    <NModal v-model:show="showCreateModal" preset="dialog" title="创建任务" style="width: 600px">
      <NForm :model="newTask">
        <NFormItem label="任务标题" required>
          <NInput v-model:value="newTask.title" placeholder="请输入任务标题" />
        </NFormItem>
        <NFormItem label="任务描述">
          <NInput
            v-model:value="newTask.description"
            type="textarea"
            placeholder="请输入任务描述（可选）"
            :rows="3"
          />
        </NFormItem>
        <NFormItem label="父任务">
          <NTreeSelect
            v-model:value="newTask.parent_id"
            :options="parentTaskTreeData"
            placeholder="选择父任务（留空为顶级任务）"
            clearable
            filterable
            :default-expand-all="false"
            :default-expanded-keys="[]"
            style="width: 100%"
          />
        </NFormItem>
        <NFormItem label="预计时间">
          <NInputNumber
            v-model:value="newTask.estimated_time_ms"
            :min="0.1"
            :step="0.5"
            clearable
            placeholder="留空表示无预期时间"
            style="width: 100%"
          >
            <template #suffix>小时</template>
          </NInputNumber>
        </NFormItem>
        <NFormItem label="截止时间">
          <NDatePicker
            v-model:formatted-value="newTask.due_at"
            type="datetime"
            format="yyyy-MM-dd HH:mm"
            value-format="yyyy-MM-dd HH:mm:ss"
            clearable
            placeholder="选择截止时间（可选）"
            style="width: 100%"
          />
        </NFormItem>
        <NFormItem label="DDL模式">
          <div style="display: flex; align-items: center; gap: 8px">
            <NCheckbox v-model:checked="newTask.is_ddl" />
            <span style="font-size: 13px; color: #666">
              启用后，超过截止时间将自动锁定任务，无法再修改
            </span>
          </div>
        </NFormItem>
      </NForm>
      <template #action>
        <NSpace>
          <NButton @click="showCreateModal = false">取消</NButton>
          <NButton type="primary" @click="handleCreateTask">创建</NButton>
        </NSpace>
      </template>
    </NModal>

    <!-- 编辑任务弹窗 -->
    <NModal v-model:show="showEditModal" preset="dialog" title="编辑任务" style="width: 600px">
      <NForm :model="editTask">
        <NFormItem label="任务标题" required>
          <NInput v-model:value="editTask.title" placeholder="请输入任务标题" />
        </NFormItem>
        <NFormItem label="任务描述">
          <NInput
            v-model:value="editTask.description"
            type="textarea"
            placeholder="请输入任务描述（可选）"
            :rows="3"
          />
        </NFormItem>
        <NFormItem label="父任务">
          <NTreeSelect
            v-model:value="editTask.parent_id"
            :options="parentTaskTreeData"
            placeholder="选择父任务（留空为顶级任务）"
            clearable
            filterable
            :default-expand-all="false"
            :default-expanded-keys="[]"
            style="width: 100%"
          />
        </NFormItem>
        <NFormItem label="状态">
          <NSelect
            v-model:value="editTask.status"
            :options="statusOptions"
            placeholder="选择任务状态"
          />
        </NFormItem>
        <NFormItem label="优先级">
          <NSelect
            v-model:value="editTask.priority"
            :options="priorityOptions"
            placeholder="选择优先级"
          />
        </NFormItem>
        <NFormItem label="预计时间">
          <NInputNumber
            v-model:value="editTask.estimated_time_ms"
            :min="0.1"
            :step="0.5"
            clearable
            placeholder="留空表示无预期时间"
            style="width: 100%"
          >
            <template #suffix>小时</template>
          </NInputNumber>
        </NFormItem>
        <NFormItem label="截止时间">
          <NDatePicker
            v-model:formatted-value="editTask.due_at"
            type="datetime"
            format="yyyy-MM-dd HH:mm"
            value-format="yyyy-MM-dd HH:mm:ss"
            clearable
            placeholder="选择截止时间（可选）"
            style="width: 100%"
          />
        </NFormItem>
        <NFormItem label="DDL模式">
          <div style="display: flex; align-items: center; gap: 8px">
            <NCheckbox v-model:checked="editTask.is_ddl" />
            <span style="font-size: 13px; color: #666">
              启用后，超过截止时间将自动锁定任务，无法再修改
            </span>
          </div>
        </NFormItem>
      </NForm>
      <template #action>
        <NSpace>
          <NButton @click="showEditModal = false">取消</NButton>
          <NButton type="primary" @click="handleUpdateTask">保存</NButton>
        </NSpace>
      </template>
    </NModal>

    <!-- 时间片信息输入弹窗 -->
    <NModal v-model:show="showTimeSliceModal" preset="dialog" title="⏱️ 记录本次工作" style="width: 500px">
      <div style="margin-bottom: 16px; padding: 12px; background: #f9fafb; border-radius: 8px">
        <div style="font-size: 12px; color: #666; margin-bottom: 4px">工作时长</div>
        <div style="font-size: 20px; font-weight: 600; color: #18a058" v-if="currentTimeSlice">
          {{ formatDuration(new Date().getTime() - new Date(currentTimeSlice.start_at).getTime()) }}
        </div>
      </div>
      <NForm :model="timeSliceForm">
        <NFormItem label="效率评分" required>
          <NSelect
            v-model:value="timeSliceForm.efficiency_score"
            :options="[
              { label: '⭐ 1 - 非常低', value: 1 },
              { label: '⭐⭐ 2 - 较低', value: 2 },
              { label: '⭐⭐⭐ 3 - 中等', value: 3 },
              { label: '⭐⭐⭐⭐ 4 - 较高', value: 4 },
              { label: '⭐⭐⭐⭐⭐ 5 - 非常高', value: 5 },
            ]"
            placeholder="选择效率评分"
          />
        </NFormItem>
        <NFormItem label="工作备注">
          <NInput
            v-model:value="timeSliceForm.note"
            type="textarea"
            placeholder="记录本次工作的内容、遇到的问题等（可选）"
            :rows="4"
          />
        </NFormItem>
      </NForm>
      <template #action>
        <NSpace>
          <NButton @click="handleTimeSliceCancel">取消</NButton>
          <NButton type="primary" @click="handleTimeSliceSubmit">保存</NButton>
        </NSpace>
      </template>
    </NModal>

    <!-- 编辑时间片弹窗 -->
    <NModal v-model:show="showEditTimeSliceModal" preset="dialog" title="✏️ 编辑时间片" style="width: 500px">
      <NForm :model="editTimeSliceForm">
        <NFormItem label="开始时间" required>
          <NDatePicker
            v-model:formatted-value="editTimeSliceForm.start_at"
            type="datetime"
            format="yyyy-MM-dd HH:mm:ss"
            value-format="yyyy-MM-dd HH:mm:ss"
            clearable
            style="width: 100%"
          />
        </NFormItem>
        <NFormItem label="结束时间" required>
          <NDatePicker
            v-model:formatted-value="editTimeSliceForm.end_at"
            type="datetime"
            format="yyyy-MM-dd HH:mm:ss"
            value-format="yyyy-MM-dd HH:mm:ss"
            clearable
            style="width: 100%"
          />
        </NFormItem>
        <NFormItem label="效率评分" required>
          <NSelect
            v-model:value="editTimeSliceForm.efficiency_score"
            :options="[
              { label: '⭐ 1 - 非常低', value: 1 },
              { label: '⭐⭐ 2 - 较低', value: 2 },
              { label: '⭐⭐⭐ 3 - 中等', value: 3 },
              { label: '⭐⭐⭐⭐ 4 - 较高', value: 4 },
              { label: '⭐⭐⭐⭐⭐ 5 - 非常高', value: 5 },
            ]"
            placeholder="选择效率评分"
          />
        </NFormItem>
        <NFormItem label="工作备注">
          <NInput
            v-model:value="editTimeSliceForm.note"
            type="textarea"
            placeholder="记录本次工作的内容、遇到的问题等（可选）"
            :rows="4"
          />
        </NFormItem>
      </NForm>
      <template #action>
        <NSpace>
          <NButton @click="showEditTimeSliceModal = false">取消</NButton>
          <NButton type="primary" @click="handleUpdateTimeSlice">保存</NButton>
        </NSpace>
      </template>
    </NModal>

    <!-- 右键菜单 -->
    <NDropdown
      placement="bottom-start"
      trigger="manual"
      :x="contextMenuX"
      :y="contextMenuY"
      :options="contextMenuOptions"
      :show="showContextMenu"
      :on-clickoutside="() => showContextMenu = false"
      @select="handleContextMenuSelect"
    />

    <!-- 回收站 -->
    <NModal
      v-model:show="showTrash"
      :mask-closable="false"
      style="width: 900px; height: 80vh"
    >
      <TrashView
        @close="showTrash = false"
        @task-restored="handleTaskRestored"
      />
    </NModal>
  </NLayout>
</template>
