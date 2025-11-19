<script setup lang="ts">
import { ref, computed } from 'vue'
import { NLayout, NLayoutHeader, NLayoutContent, NTabs, NTabPane, NButton, NSpin } from 'naive-ui'
import { useTasks } from '@/composables/useTasks'
import type { Task } from '@/types'

// 使用任务数据
const {
  tasks,
  taskTree,
  workingTask,
  stats,
  isLoading,
  selectTask,
  toggleTaskWork,
  getTaskStatusIcon,
  getPriorityColor
} = useTasks()

// UI状态
const activeTab = ref('tasks')

// 方法
const handleTabChange = (value: string) => {
  activeTab.value = value
  console.log('切换到标签:', value)
}

// 处理任务点击
const handleTaskClick = (task: Task) => {
  selectTask(task)
  console.log('点击任务:', task.title)
  // TODO: 打开任务详情抽屉
}

// 处理任务工作切换
const handleWorkToggle = async (task: Task, event: Event) => {
  event.stopPropagation() // 防止触发任务点击
  await toggleTaskWork(task)
}

// 获取任务显示文本
const getTaskLabel = (task: Task): string => {
  const icon = getTaskStatusIcon(task)
  return `${icon} ${task.title}`
}

// 扁平化任务树（用于移动端列表显示）
const flatTasks = computed(() => {
  const flatten = (tasks: Task[], depth = 0): Array<Task & { depth: number }> => {
    return tasks.reduce((acc, task) => {
      acc.push({ ...task, depth })
      if (task.children && task.children.length > 0) {
        acc.push(...flatten(task.children, depth + 1))
      }
      return acc
    }, [] as Array<Task & { depth: number }>)
  }
  
  return flatten(taskTree.value)
})

// 格式化时间（毫秒转小时分钟）
const formatTime = (ms: number): string => {
  if (!ms) return '0分钟'
  const minutes = Math.floor(ms / (1000 * 60))
  const hours = Math.floor(minutes / 60)
  const remainingMinutes = minutes % 60
  
  if (hours > 0) {
    return `${hours}小时${remainingMinutes}分钟`
  }
  return `${remainingMinutes}分钟`
}
</script>

<template>
  <div class="mobile-layout">
    <!-- iPhone 顶部 Header -->
    <NLayoutHeader class="mobile-header">
      <div class="header-content">
        <h1 class="app-title">⏱️ TBox</h1>
        <div class="header-actions">
          <NButton circle size="small">🗑️</NButton>
          <NButton circle size="small" type="primary">➕</NButton>
        </div>
      </div>
    </NLayoutHeader>

    <!-- 内容区域 -->
    <NLayoutContent class="mobile-content">
      <NTabs 
        v-model:value="activeTab" 
        type="segment"
        size="large"
        class="mobile-tabs"
        placement="bottom"
        @update:value="handleTabChange"
      >
        <!-- 任务列表 Tab -->
        <NTabPane name="tasks" :tab="`📋 任务${workingTask ? ' (工作中)' : ''}`">
          <div class="tab-panel">
            <NSpin :show="isLoading" description="加载中...">
              <div v-if="flatTasks.length > 0" class="task-list">
                <div 
                  v-for="task in flatTasks" 
                  :key="task.id"
                  class="task-item"
                  :class="{
                    working: task.execution_state === 'working',
                    completed: task.status === 'completed'
                  }"
                  :style="{
                    marginLeft: `${task.depth * 16}px`,
                    borderLeft: task.depth > 0 ? '2px solid #e0e0e0' : 'none',
                    borderLeftColor: getPriorityColor(task.priority)
                  }"
                  @click="handleTaskClick(task)"
                >
                  <div class="task-main">
                    <div class="task-content">
                      <span class="task-label">{{ getTaskLabel(task) }}</span>
                      <div class="task-meta">
                        <span class="task-priority" :style="{ color: getPriorityColor(task.priority) }">
                          P{{ task.priority }}
                        </span>
                        <span v-if="task.total_logged_ms > 0" class="task-time">
                          {{ formatTime(task.total_logged_ms) }}
                        </span>
                      </div>
                    </div>
                    
                    <!-- 工作按钮 -->
                    <div class="task-actions">
                      <NButton
                        v-if="task.status !== 'completed'"
                        size="small"
                        :type="task.execution_state === 'working' ? 'error' : 'primary'"
                        circle
                        @click="handleWorkToggle(task, $event)"
                      >
                        {{ task.execution_state === 'working' ? '⏸️' : '▶️' }}
                      </NButton>
                    </div>
                  </div>
                  
                  <!-- 进度信息 -->
                  <div v-if="task.estimated_time_ms && task.total_logged_ms" class="task-progress">
                    <div class="progress-text">
                      {{ Math.round((task.total_logged_ms / task.estimated_time_ms) * 100) }}% 
                      ({{ formatTime(task.total_logged_ms) }} / {{ formatTime(task.estimated_time_ms) }})
                    </div>
                  </div>
                </div>
              </div>
              
              <div v-else-if="!isLoading" class="empty-state">
                <div class="empty-icon">📝</div>
                <div class="empty-text">还没有任务</div>
                <div class="empty-subtitle">点击右上角 ➕ 创建第一个任务</div>
              </div>
            </NSpin>
          </div>
        </NTabPane>

        <!-- 统计 Tab -->
        <NTabPane name="stats" tab="📊 统计">
          <div class="tab-panel">
            <div class="temp-content">
              <h2>📊 统计数据</h2>
              
              <div class="stats-grid">
                <div class="stat-card">
                  <div class="stat-title">总任务数</div>
                  <div class="stat-value">{{ stats.total }}</div>
                </div>
                <div class="stat-card">
                  <div class="stat-title">已完成</div>
                  <div class="stat-value" style="color: #18a058;">{{ stats.completed }}</div>
                </div>
                <div class="stat-card">
                  <div class="stat-title">进行中</div>
                  <div class="stat-value" style="color: #f0a020;">{{ stats.inProgress }}</div>
                </div>
                <div class="stat-card">
                  <div class="stat-title">待开始</div>
                  <div class="stat-value" style="color: #909399;">{{ stats.pending }}</div>
                </div>
              </div>
              
              <div v-if="workingTask" class="current-work">
                <h3>⏱️ 当前工作</h3>
                <div class="work-card">
                  <div class="work-title">{{ workingTask.title }}</div>
                  <div class="work-time">已工作: {{ formatTime(workingTask.total_logged_ms) }}</div>
                </div>
              </div>
              
              <div class="chart-placeholder">
                <p style="color: #666; text-align: center; padding: 40px;">
                  📊 图表功能开发中...
                </p>
              </div>
            </div>
          </div>
        </NTabPane>

        <!-- 我的 Tab -->
        <NTabPane name="profile" tab="👤 我的">
          <div class="tab-panel">
            <div class="temp-content">
              <h2>👤 个人中心</h2>
              <div class="menu-demo">
                <div class="menu-item">🗑️ 回收站</div>
                <div class="menu-item">⚙️ 设置</div>
                <div class="menu-item">📖 使用说明</div>
                <div class="menu-item">ℹ️ 关于 TBox</div>
              </div>
            </div>
          </div>
        </NTabPane>
      </NTabs>
    </NLayoutContent>
  </div>
</template>

<style scoped>
.mobile-layout {
  display: flex;
  flex-direction: column;
  height: 100vh;
  height: 100dvh; /* iPhone Dynamic Viewport Height */
  overflow: hidden;
  background: #f5f5f5;
}

.mobile-header {
  flex-shrink: 0;
  background: white;
  border-bottom: 1px solid #e0e0e0;
  /* iPhone Safe Area 支持 */
  padding-top: env(safe-area-inset-top);
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
}

.header-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 20px;
  height: 56px;
}

.app-title {
  font-size: 20px;
  font-weight: 600;
  color: #18a058;
  margin: 0;
}

.header-actions {
  display: flex;
  gap: 12px;
}

.mobile-content {
  flex: 1;
  overflow: hidden;
  background: white;
}

.mobile-tabs {
  height: 100%;
  /* 确保底部 tab 在 Safe Area 上方 */
  padding-bottom: env(safe-area-inset-bottom);
}

:deep(.n-tabs .n-tabs-nav) {
  /* 底部导航样式优化 */
  background: white;
  border-top: 1px solid #e0e0e0;
  padding: 8px 0;
  /* iPhone 毛玻璃效果 */
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
}

:deep(.n-tabs .n-tabs-wrapper) {
  /* 内容区域充满剩余空间 */
  flex: 1;
  overflow: hidden;
}

:deep(.n-tab-pane) {
  height: 100%;
  overflow: auto;
  /* iPhone 滚动优化 */
  -webkit-overflow-scrolling: touch;
}

.tab-panel {
  height: 100%;
  padding: 16px;
}

/* 任务列表样式 */
.task-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.task-item {
  background: white;
  border-radius: 12px;
  border: 1px solid #e0e0e0;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
  transition: all 0.2s ease;
  overflow: hidden;
  /* iPhone 最小触摸区域 */
  min-height: 60px;
}

.task-item:active {
  transform: scale(0.98);
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1);
}

.task-item.working {
  border-color: #18a058;
  background: linear-gradient(135deg, #f6ffed 0%, #ffffff 100%);
}

.task-item.completed {
  opacity: 0.7;
  background: #f9f9f9;
}

.task-main {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 20px;
  gap: 12px;
}

.task-content {
  flex: 1;
  min-width: 0; /* 允许内容缩小 */
}

.task-label {
  font-size: 16px;
  font-weight: 500;
  color: #333;
  display: block;
  margin-bottom: 4px;
  /* 文字截断 */
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
}

.task-meta {
  display: flex;
  gap: 12px;
  align-items: center;
}

.task-priority {
  font-size: 12px;
  font-weight: 600;
  padding: 2px 6px;
  background: rgba(0, 0, 0, 0.05);
  border-radius: 4px;
}

.task-time {
  font-size: 12px;
  color: #666;
}

.task-actions {
  flex-shrink: 0;
}

.task-progress {
  padding: 8px 20px 16px;
  border-top: 1px solid #f0f0f0;
}

.progress-text {
  font-size: 12px;
  color: #666;
  text-align: right;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  text-align: center;
}

.empty-icon {
  font-size: 48px;
  margin-bottom: 16px;
}

.empty-text {
  font-size: 18px;
  font-weight: 500;
  color: #333;
  margin-bottom: 8px;
}

.empty-subtitle {
  font-size: 14px;
  color: #666;
}

/* 统计页面样式 */
.temp-content {
  max-width: 100%;
}

.temp-content h2 {
  margin-bottom: 20px;
  color: #333;
  font-size: 24px;
}

.temp-content h3 {
  margin: 24px 0 12px;
  color: #333;
  font-size: 18px;
}

.stats-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  margin-bottom: 24px;
}

.stat-card {
  background: white;
  padding: 20px;
  border-radius: 12px;
  border: 1px solid #e0e0e0;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
  text-align: center;
}

.stat-title {
  font-size: 14px;
  color: #666;
  margin-bottom: 8px;
}

.stat-value {
  font-size: 24px;
  font-weight: 600;
  color: #333;
}

.current-work {
  margin-bottom: 24px;
}

.work-card {
  background: linear-gradient(135deg, #f6ffed 0%, #ffffff 100%);
  padding: 16px 20px;
  border-radius: 12px;
  border: 1px solid #b7eb8f;
}

.work-title {
  font-size: 16px;
  font-weight: 500;
  color: #333;
  margin-bottom: 4px;
}

.work-time {
  font-size: 14px;
  color: #52c41a;
}

.chart-placeholder {
  margin-top: 24px;
  background: white;
  border-radius: 12px;
  border: 2px dashed #e0e0e0;
}

/* 菜单项样式 */
.menu-demo {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.menu-item {
  background: white;
  padding: 16px 20px;
  border-radius: 12px;
  border: 1px solid #e0e0e0;
  font-size: 16px;
  /* iPhone 最小触摸区域 */
  min-height: 56px;
  display: flex;
  align-items: center;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
  transition: all 0.2s ease;
  cursor: pointer;
}

.menu-item:active {
  transform: scale(0.98);
  background: #f0f0f0;
}

/* iPhone 专用优化 */
@media (max-width: 430px) {
  .mobile-layout {
    /* 确保在iPhone上正确显示 */
    min-height: -webkit-fill-available;
  }
  
  .header-content {
    padding: 12px 16px;
  }
  
  .tab-panel {
    padding: 12px;
  }
  
  .task-main {
    padding: 14px 16px;
  }
  
  .stats-grid {
    grid-template-columns: 1fr;
    gap: 8px;
  }
}

/* 触摸优化 */
@media (hover: none) and (pointer: coarse) {
  .task-item:hover,
  .menu-item:hover {
    background: white;
  }
  
  .task-item.working:hover {
    background: linear-gradient(135deg, #f6ffed 0%, #ffffff 100%);
  }
}
</style>