<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { NCard, NGrid, NGridItem, NStatistic, NSpace } from 'naive-ui'
import { projectsApi } from '@/api/projects'
import type { Project } from '@/types'

const projects = ref<Project[]>([])
const stats = ref({
  totalProjects: 0,
  totalTasks: 0,
  totalTime: 0,
  completedTasks: 0,
})

const loadData = async () => {
  try {
    const response = await projectsApi.getAll()
    projects.value = response.data
    
    stats.value.totalProjects = response.data.length
    stats.value.totalTasks = response.data.reduce((sum, p) => sum + p.tasks.length, 0)
    stats.value.totalTime = response.data.reduce((sum, p) => sum + p.total_logged_ms, 0)
    stats.value.completedTasks = response.data.reduce(
      (sum, p) => sum + p.tasks.filter(t => t.status === 'completed').length,
      0
    )
  } catch (error) {
    console.error('Failed to load data:', error)
  }
}

onMounted(() => {
  loadData()
})
</script>

<template>
  <div>
    <h1 style="margin-bottom: 24px">Dashboard</h1>
    
    <NGrid :x-gap="24" :y-gap="24" :cols="4">
      <NGridItem>
        <NCard>
          <NStatistic label="项目总数" :value="stats.totalProjects" />
        </NCard>
      </NGridItem>
      <NGridItem>
        <NCard>
          <NStatistic label="任务总数" :value="stats.totalTasks" />
        </NCard>
      </NGridItem>
      <NGridItem>
        <NCard>
          <NStatistic label="已完成任务" :value="stats.completedTasks" />
        </NCard>
      </NGridItem>
      <NGridItem>
        <NCard>
          <NStatistic
            label="总用时"
            :value="(stats.totalTime / 3600000).toFixed(1) + 'h'"
          />
        </NCard>
      </NGridItem>
    </NGrid>

    <NCard title="最近项目" style="margin-top: 24px">
      <NSpace vertical>
        <div v-for="project in projects" :key="project.id">
          <h3>{{ project.name }}</h3>
          <p>{{ project.description }}</p>
          <p>任务数: {{ project.tasks.length }}</p>
        </div>
        <p v-if="projects.length === 0">暂无项目</p>
      </NSpace>
    </NCard>
  </div>
</template>
