<script setup lang="ts">
import { ref, onMounted, watch, computed } from 'vue'
import * as echarts from 'echarts'
import type { TimeSlice } from '@/types'

interface Props {
  timeSlices: TimeSlice[]
}

const props = defineProps<Props>()

// 定义 emit
const emit = defineEmits<{
  taskClick: [taskId: string]
}>()

const chartRef = ref<HTMLDivElement>()
let chartInstance: echarts.ECharts | null = null

// 日期导航状态
const selectedDate = ref(new Date())

// 判断是否是今天
const isToday = computed(() => {
  const today = new Date()
  const selected = selectedDate.value
  return today.getFullYear() === selected.getFullYear() &&
         today.getMonth() === selected.getMonth() &&
         today.getDate() === selected.getDate()
})

// 格式化日期显示
const formatDisplayDate = (date: Date): string => {
  const year = date.getFullYear()
  const month = (date.getMonth() + 1).toString().padStart(2, '0')
  const day = date.getDate().toString().padStart(2, '0')
  const weekdays = ['日', '一', '二', '三', '四', '五', '六']
  const weekday = weekdays[date.getDay()]
  return `${year}-${month}-${day} (${weekday})`
}

// 导航到前一天
const goToPreviousDay = () => {
  const newDate = new Date(selectedDate.value)
  newDate.setDate(newDate.getDate() - 1)
  selectedDate.value = newDate
}

// 导航到后一天
const goToNextDay = () => {
  if (isToday.value) return // 今天不能再往后
  const newDate = new Date(selectedDate.value)
  newDate.setDate(newDate.getDate() + 1)
  selectedDate.value = newDate
}

// 回到今天
const goToToday = () => {
  selectedDate.value = new Date()
}

// 效率评分对应的颜色
const getEfficiencyColor = (score: number): string => {
  const colors: Record<number, string> = {
    1: '#ef4444', // 红色 - 非常低
    2: '#f97316', // 橙色 - 较低
    3: '#eab308', // 黄色 - 中等
    4: '#84cc16', // 浅绿 - 较高
    5: '#22c55e', // 深绿 - 非常高
  }
  return colors[score] || '#9ca3af'
}

// 时间段类型
interface TimeSegment {
  startMs: number  // 从00:00开始的毫秒数
  endMs: number
  type: 'idle' | 'working' | 'future'
  efficiency?: number
  label: string
  color: string
  taskId?: string  // 任务ID（仅工作时间段有）
}

// 将时间片数据转换为动态时间段
const processTimeSlices = () => {
  // 使用选定的日期
  const displayDate = selectedDate.value
  const dayStart = new Date(displayDate.getFullYear(), displayDate.getMonth(), displayDate.getDate())
  const dayMs = 24 * 60 * 60 * 1000

  // 计算当前时间点（如果查看的是今天）
  const now = new Date()
  const currentMs = isToday.value
    ? (now.getHours() * 60 + now.getMinutes()) * 60 * 1000 + now.getSeconds() * 1000
    : dayMs // 如果是历史日期，当前时间就是一天结束

  // 过滤出选定日期的时间片，并转换为从00:00开始的毫秒数
  // 只处理已完成的时间片（end_at 不为 null）
  const todaySlices = props.timeSlices
    .filter(slice => {
      if (!slice.end_at) return false // 过滤掉未完成的时间片
      const sliceStart = new Date(slice.start_at)
      return sliceStart >= dayStart && sliceStart < new Date(dayStart.getTime() + dayMs)
    })
    .map(slice => {
      const start = new Date(slice.start_at)
      const end = new Date(slice.end_at!)
      const startMs = (start.getHours() * 60 + start.getMinutes()) * 60 * 1000 + start.getSeconds() * 1000
      const endMs = (end.getHours() * 60 + end.getMinutes()) * 60 * 1000 + end.getSeconds() * 1000
      return {
        startMs,
        endMs: Math.min(endMs, dayMs), // 确保不超过24小时
        efficiency: slice.efficiency_score!,
        taskId: slice.task_id,
      }
    })
    .sort((a, b) => a.startMs - b.startMs)

  // 构建时间段
  const segments: TimeSegment[] = []
  let lastEndMs = 0

  for (const slice of todaySlices) {
    // 添加空闲时间段（如果有间隙）
    if (slice.startMs > lastEndMs) {
      const gapMs = slice.startMs - lastEndMs
      if (lastEndMs < currentMs) {
        // 过去的空闲时间
        const idleEndMs = Math.min(slice.startMs, currentMs)
        if (idleEndMs > lastEndMs) {
          segments.push({
            startMs: lastEndMs,
            endMs: idleEndMs,
            type: 'idle',
            label: formatMsToTime(lastEndMs) + '-' + formatMsToTime(idleEndMs) + ' 空闲',
            color: '#d1d5db',
          })
        }
      }

      // 如果间隙跨越当前时间，添加未来时间段
      if (slice.startMs > currentMs && lastEndMs < currentMs) {
        segments.push({
          startMs: currentMs,
          endMs: slice.startMs,
          type: 'future',
          label: formatMsToTime(currentMs) + '-' + formatMsToTime(slice.startMs) + ' 未来',
          color: '#ffffff',
        })
      }
    }

    // 添加工作时间段
    if (slice.endMs > lastEndMs) {
      const workStartMs = Math.max(slice.startMs, lastEndMs)
      const workEndMs = slice.endMs

      if (workStartMs < currentMs) {
        // 已完成的工作
        const actualEndMs = Math.min(workEndMs, currentMs)
        segments.push({
          startMs: workStartMs,
          endMs: actualEndMs,
          type: 'working',
          efficiency: slice.efficiency,
          label: formatMsToTime(workStartMs) + '-' + formatMsToTime(actualEndMs) + ' 效率' + slice.efficiency + '/5',
          color: getEfficiencyColor(slice.efficiency),
          taskId: slice.taskId,
        })
      }

      // 如果工作跨越当前时间，剩余部分算未来
      if (workEndMs > currentMs && slice.startMs < currentMs) {
        segments.push({
          startMs: currentMs,
          endMs: workEndMs,
          type: 'future',
          label: formatMsToTime(currentMs) + '-' + formatMsToTime(workEndMs) + ' 未来',
          color: '#ffffff',
        })
      }

      lastEndMs = Math.max(lastEndMs, workEndMs)
    }
  }

  // 添加最后的空闲或未来时间段
  if (lastEndMs < dayMs) {
    if (lastEndMs < currentMs) {
      // 剩余的空闲时间
      segments.push({
        startMs: lastEndMs,
        endMs: currentMs,
        type: 'idle',
        label: formatMsToTime(lastEndMs) + '-' + formatMsToTime(currentMs) + ' 空闲',
        color: '#d1d5db',
      })
      lastEndMs = currentMs
    }

    // 未来时间
    if (lastEndMs < dayMs) {
      segments.push({
        startMs: lastEndMs,
        endMs: dayMs,
        type: 'future',
        label: formatMsToTime(lastEndMs) + '-' + formatMsToTime(dayMs) + ' 未来',
        color: '#ffffff',
      })
    }
  }

  // 转换为 ECharts 数据格式
  const data = segments.map(segment => {
    const durationMs = segment.endMs - segment.startMs
    const durationMinutes = Math.round(durationMs / 60000)

    return {
      value: durationMinutes, // 使用实际时长作为值
      name: segment.label + ` (${formatDuration(durationMs)})`,
      itemStyle: {
        color: segment.color,
        borderColor: '#ffffff',
        borderWidth: 1,
      },
      taskId: segment.taskId, // 保存任务ID
      segmentType: segment.type, // 保存段类型
    }
  })

  return data
}

// 格式化毫秒为时间字符串 HH:MM
const formatMsToTime = (ms: number): string => {
  const totalSeconds = Math.floor(ms / 1000)
  const hours = Math.floor(totalSeconds / 3600)
  const minutes = Math.floor((totalSeconds % 3600) / 60)
  return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}`
}

// 格式化时长
const formatDuration = (ms: number): string => {
  const totalMinutes = Math.floor(ms / 60000)
  const hours = Math.floor(totalMinutes / 60)
  const minutes = totalMinutes % 60

  if (hours > 0) {
    return `${hours}h${minutes}min`
  }
  return `${minutes}min`
}

// 初始化图表
const initChart = () => {
  if (!chartRef.value) return

  chartInstance = echarts.init(chartRef.value)

  const data = processTimeSlices()

  const option: echarts.EChartsOption = {
    tooltip: {
      trigger: 'item',
      formatter: '{b}',
    },
    series: [
      {
        type: 'pie',
        radius: ['40%', '70%'], // 环形图
        avoidLabelOverlap: false,
        label: {
          show: false,
        },
        labelLine: {
          show: false,
        },
        data: data,
        emphasis: {
          itemStyle: {
            shadowBlur: 10,
            shadowOffsetX: 0,
            shadowColor: 'rgba(0, 0, 0, 0.5)',
          },
        },
      },
    ],
    graphic: [
      {
        type: 'text',
        left: 'center',
        top: 'center',
        style: {
          text: 'Day\nEfficiency',
          textAlign: 'center',
          fill: '#333',
          fontSize: 16,
          fontWeight: 'bold',
        },
      },
    ],
  }

  chartInstance.setOption(option)

  // 添加点击事件
  chartInstance.on('click', (params: any) => {
    if (params.componentType === 'series' && params.data) {
      const taskId = params.data.taskId
      const segmentType = params.data.segmentType

      // 只有工作时间段才有任务ID
      if (segmentType === 'working' && taskId) {
        emit('taskClick', taskId)
      }
    }
  })
}

// 更新图表
const updateChart = () => {
  if (!chartInstance) return

  const data = processTimeSlices()
  chartInstance.setOption({
    series: [
      {
        data: data,
      },
    ],
  })
}

// 监听 props 变化
watch(() => props.timeSlices, () => {
  updateChart()
}, { deep: true })

// 监听日期变化
watch(selectedDate, () => {
  updateChart()
})

onMounted(() => {
  initChart()

  // 响应式调整大小
  window.addEventListener('resize', () => {
    chartInstance?.resize()
  })
})

// 暴露方法供父组件调用
defineExpose({
  goToDate: (dateStr: string) => {
    // dateStr 格式: YYYY-MM-DD
    const [year, month, day] = dateStr.split('-').map(Number)
    selectedDate.value = new Date(year, month - 1, day)
  }
})
</script>

<template>
  <div style="display: flex; flex-direction: column; height: 100%;">
    <!-- 日期导航栏 -->
    <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; padding: 0 16px;">
      <!-- 左箭头 -->
      <button
        @click="goToPreviousDay"
        style="cursor: pointer; border: none; background: none; font-size: 24px; color: #666; padding: 4px 8px; user-select: none;"
        title="前一天"
      >
        ‹
      </button>

      <!-- 日期显示 -->
      <div style="display: flex; flex-direction: column; align-items: center; gap: 4px;">
        <span style="font-size: 13px; color: #666; font-weight: 500;">
          {{ formatDisplayDate(selectedDate) }}
        </span>
        <button
          v-if="!isToday"
          @click="goToToday"
          style="cursor: pointer; border: 1px solid #d1d5db; background: white; color: #666; font-size: 11px; padding: 2px 8px; border-radius: 4px;"
        >
          回到今天
        </button>
      </div>

      <!-- 右箭头 -->
      <button
        @click="goToNextDay"
        :disabled="isToday"
        :style="{
          cursor: isToday ? 'not-allowed' : 'pointer',
          border: 'none',
          background: 'none',
          fontSize: '24px',
          color: isToday ? '#d1d5db' : '#666',
          padding: '4px 8px',
          userSelect: 'none'
        }"
        title="后一天"
      >
        ›
      </button>
    </div>

    <!-- 饼图 -->
    <div ref="chartRef" style="flex: 1; width: 100%;"></div>
  </div>
</template>

<style scoped>
</style>
