<script setup lang="ts">
import { ref, onMounted, h } from 'vue'
import { useMessage } from 'naive-ui'
import { NCard, NSpace, NButton, NDataTable } from 'naive-ui'
import type { DataTableColumns } from 'naive-ui'
import { projectsApi } from '@/api/projects'
import type { Project } from '@/types'

const message = useMessage()
const loading = ref(false)
const projects = ref<Project[]>([])

const columns: DataTableColumns<Project> = [
  { title: '项目名称', key: 'name' },
  { title: '描述', key: 'description' },
  {
    title: '计划时间',
    key: 'planned_time_ms',
    render: (row) => (row.planned_time_ms / 3600000).toFixed(1) + 'h',
  },
  {
    title: '已用时间',
    key: 'total_logged_ms',
    render: (row) => (row.total_logged_ms / 3600000).toFixed(1) + 'h',
  },
  {
    title: '任务数',
    key: 'tasks',
    render: (row) => row.tasks.length,
  },
]

const loadProjects = async () => {
  loading.value = true
  try {
    const response = await projectsApi.getAll()
    projects.value = response.data
  } catch (error) {
    message.error('加载项目失败')
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  loadProjects()
})
</script>

<template>
  <div>
    <NCard title="项目管理">
      <template #header-extra>
        <NButton type="primary">创建项目</NButton>
      </template>
      
      <NDataTable
        :columns="columns"
        :data="projects"
        :loading="loading"
        :bordered="false"
      />
    </NCard>
  </div>
</template>
