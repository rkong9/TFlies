<template>
  <div class="trash-view">
    <!-- 头部 -->
    <div class="trash-header">
      <div class="header-left">
        <h2>回收站</h2>
        <n-tag v-if="deletedTasks.length > 0" :bordered="false" type="info">
          {{ deletedTasks.length }} 个项目
        </n-tag>
      </div>
      <div class="header-right">
        <n-button
          v-if="deletedTasks.length > 0"
          type="error"
          ghost
          @click="handleEmptyTrash"
        >
          清空回收站
        </n-button>
        <n-button @click="$emit('close')">
          关闭
        </n-button>
      </div>
    </div>

    <!-- 回收站列表 -->
    <div class="trash-content">
      <n-spin :show="loading">
        <n-empty
          v-if="!loading && deletedTasks.length === 0"
          description="回收站是空的"
          style="margin-top: 100px"
        >
          <template #icon>
            <div style="font-size: 64px">🗑️</div>
          </template>
        </n-empty>

        <div v-else class="trash-list">
          <div
            v-for="item in deletedTasks"
            :key="item.id"
            class="trash-item"
          >
            <div class="item-header">
              <div class="item-title">
                {{ item.title }}
                <n-tag
                  v-if="item.total_descendants > 1"
                  :bordered="false"
                  size="small"
                  type="info"
                >
                  包含 {{ item.total_descendants }} 个任务
                </n-tag>
              </div>
              <div class="item-actions">
                <n-button
                  type="success"
                  size="small"
                  ghost
                  @click="handleRestore(item)"
                >
                  恢复
                </n-button>
                <n-button
                  type="error"
                  size="small"
                  ghost
                  @click="handlePermanentDelete(item)"
                >
                  永久删除
                </n-button>
              </div>
            </div>

            <div class="item-info">
              <div class="info-item">
                <span class="label">删除时间：</span>
                <span class="value">{{ formatDate(item.deleted_at) }}</span>
              </div>
              <div class="info-item">
                <span class="label">过期时间：</span>
                <span class="value" :class="{ 'expiring-soon': isExpiringSoon(item.expiry_at) }">
                  {{ formatDate(item.expiry_at) }}
                  <span class="countdown">({{ getExpiryCountdown(item.expiry_at) }})</span>
                </span>
              </div>
            </div>
          </div>
        </div>
      </n-spin>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useMessage, useDialog, NButton, NTag, NSpin, NEmpty } from 'naive-ui'
import { tasksApi } from '@/api'
import type { DeletedTask } from '@/types'

// Emits
const emit = defineEmits<{
  close: []
  taskRestored: []
}>()

// State
const message = useMessage()
const dialog = useDialog()
const loading = ref(false)
const deletedTasks = ref<DeletedTask[]>([])

// 加载回收站列表
const loadTrash = async () => {
  loading.value = true
  try {
    const { data } = await tasksApi.trash.list()
    deletedTasks.value = data
  } catch (error: any) {
    message.error(error.response?.data?.detail || '加载回收站失败')
  } finally {
    loading.value = false
  }
}

// 恢复任务
const handleRestore = (item: DeletedTask) => {
  dialog.warning({
    title: '确认恢复',
    content: `确定要恢复任务「${item.title}」吗？${
      item.total_descendants > 1
        ? `这将恢复 ${item.total_descendants} 个任务（包括所有子任务）。`
        : ''
    }`,
    positiveText: '确定恢复',
    negativeText: '取消',
    onPositiveClick: async () => {
      try {
        await tasksApi.trash.restore(item.id)
        message.success('任务已恢复')
        await loadTrash()
        emit('taskRestored')
      } catch (error: any) {
        message.error(error.response?.data?.detail || '恢复失败')
      }
    },
  })
}

// 永久删除
const handlePermanentDelete = (item: DeletedTask) => {
  dialog.error({
    title: '⚠️ 永久删除',
    content: `确定要永久删除任务「${item.title}」吗？${
      item.total_descendants > 1
        ? `这将永久删除 ${item.total_descendants} 个任务（包括所有子任务和时间片）。`
        : ''
    }\n\n此操作不可恢复！`,
    positiveText: '确定删除',
    negativeText: '取消',
    onPositiveClick: async () => {
      try {
        await tasksApi.trash.permanentDelete(item.id)
        message.success('已永久删除')
        await loadTrash()
      } catch (error: any) {
        message.error(error.response?.data?.detail || '删除失败')
      }
    },
  })
}

// 清空回收站
const handleEmptyTrash = () => {
  const totalTasks = deletedTasks.value.reduce((sum, item) => sum + item.total_descendants, 0)

  dialog.error({
    title: '⚠️ 清空回收站',
    content: `确定要清空回收站吗？这将永久删除 ${deletedTasks.value.length} 个项目，共 ${totalTasks} 个任务（包括所有子任务和时间片）。\n\n此操作不可恢复！`,
    positiveText: '确定清空',
    negativeText: '取消',
    onPositiveClick: async () => {
      try {
        const { data } = await tasksApi.trash.empty()
        message.success(`已清空回收站，删除了 ${data.deleted_count} 个项目`)
        deletedTasks.value = []
      } catch (error: any) {
        message.error(error.response?.data?.detail || '清空失败')
      }
    },
  })
}

// 格式化日期
const formatDate = (dateStr: string) => {
  return new Date(dateStr).toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  })
}

// 获取过期倒计时
const getExpiryCountdown = (expiryAt: string) => {
  const now = new Date()
  const expiry = new Date(expiryAt)
  const diffMs = expiry.getTime() - now.getTime()

  if (diffMs < 0) {
    return '已过期'
  }

  const days = Math.floor(diffMs / (1000 * 60 * 60 * 24))
  const hours = Math.floor((diffMs % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60))

  if (days > 0) {
    return `${days}天后过期`
  } else if (hours > 0) {
    return `${hours}小时后过期`
  } else {
    return '即将过期'
  }
}

// 是否即将过期（3天内）
const isExpiringSoon = (expiryAt: string) => {
  const now = new Date()
  const expiry = new Date(expiryAt)
  const diffMs = expiry.getTime() - now.getTime()
  const diffDays = diffMs / (1000 * 60 * 60 * 24)

  return diffDays < 3 && diffDays > 0
}

onMounted(() => {
  loadTrash()
})
</script>

<style scoped>
.trash-view {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #fff;
}

.trash-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 20px 24px;
  border-bottom: 1px solid #e8e8e8;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 12px;
}

.header-left h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 600;
}

.header-right {
  display: flex;
  gap: 12px;
}

.trash-content {
  flex: 1;
  overflow-y: auto;
  padding: 24px;
}

.trash-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.trash-item {
  background: #fafafa;
  border: 1px solid #e8e8e8;
  border-radius: 8px;
  padding: 16px;
  transition: all 0.2s;
}

.trash-item:hover {
  border-color: #d0d0d0;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.item-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.item-title {
  font-size: 16px;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 8px;
}

.item-actions {
  display: flex;
  gap: 8px;
}

.item-info {
  display: flex;
  flex-direction: column;
  gap: 6px;
  font-size: 14px;
  color: #666;
}

.info-item {
  display: flex;
  align-items: center;
}

.label {
  min-width: 80px;
  color: #999;
}

.value {
  color: #333;
}

.value.expiring-soon {
  color: #f5a623;
  font-weight: 500;
}

.countdown {
  margin-left: 8px;
  font-size: 12px;
  color: #999;
}
</style>
