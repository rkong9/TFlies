<script setup lang="ts">
import { ref, onMounted, watch } from 'vue'
import * as echarts from 'echarts'
import type { TimeSlice } from '@/types'

interface Props {
  timeSlices: TimeSlice[]
}

const props = defineProps<Props>()

// 定义 emit
const emit = defineEmits<{
  dateClick: [date: string]
}>()

const chartRef = ref<HTMLDivElement>()
let chartInstance: echarts.ECharts | null = null

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
  startHour: number  // 从00:00开始的小时数
  endHour: number
  type: 'idle' | 'working' | 'future'
  efficiency?: number
  label: string
  color: string
}

// 格式化小时为时间字符串 HH:MM
const formatHourToTime = (hour: number): string => {
  const h = Math.floor(hour)
  const m = Math.floor((hour - h) * 60)
  return `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}`
}

// 获取最近7天的日期（包括今天）
const getLast7Days = (): Date[] => {
  const days: Date[] = []
  const today = new Date()

  for (let i = 6; i >= 0; i--) {
    const date = new Date(today)
    date.setDate(date.getDate() - i)
    date.setHours(0, 0, 0, 0)
    days.push(date)
  }

  return days
}

// 格式化日期为 MM-DD
const formatDate = (date: Date): string => {
  const month = (date.getMonth() + 1).toString().padStart(2, '0')
  const day = date.getDate().toString().padStart(2, '0')
  return `${month}-${day}`
}

// 格式化星期
const formatWeekday = (date: Date): string => {
  const weekdays = ['日', '一', '二', '三', '四', '五', '六']
  return weekdays[date.getDay()]
}

// 处理单天的时间片，类似饼图逻辑
const processDayTimeSlices = (day: Date): TimeSegment[] => {
  const now = new Date()
  const dayStart = new Date(day)
  const dayEnd = new Date(day)
  dayEnd.setDate(dayEnd.getDate() + 1)

  // 判断是否是今天
  const isToday = day.toDateString() === now.toDateString()
  const currentHour = isToday ? (now.getHours() + now.getMinutes() / 60 + now.getSeconds() / 3600) : 24

  // 过滤出这一天的时间片
  // 只处理已完成的时间片（end_at 不为 null）
  const todaySlices = props.timeSlices
    .filter(slice => {
      if (!slice.end_at) return false // 过滤掉未完成的时间片
      const sliceStart = new Date(slice.start_at)
      return sliceStart >= dayStart && sliceStart < dayEnd
    })
    .map(slice => {
      const start = new Date(slice.start_at)
      const end = new Date(slice.end_at!)
      const startHour = start.getHours() + start.getMinutes() / 60 + start.getSeconds() / 3600
      const endHour = Math.min(end.getHours() + end.getMinutes() / 60 + end.getSeconds() / 3600, 24)
      return {
        startHour,
        endHour: Math.min(endHour, 24),
        efficiency: slice.efficiency_score!,
      }
    })
    .sort((a, b) => a.startHour - b.startHour)

  // 构建时间段
  const segments: TimeSegment[] = []
  let lastEndHour = 0

  for (const slice of todaySlices) {
    // 添加空闲时间段（如果有间隙）
    if (slice.startHour > lastEndHour) {
      if (lastEndHour < currentHour) {
        // 过去的空闲时间
        const idleEndHour = Math.min(slice.startHour, currentHour)
        if (idleEndHour > lastEndHour) {
          segments.push({
            startHour: lastEndHour,
            endHour: idleEndHour,
            type: 'idle',
            label: formatHourToTime(lastEndHour) + '-' + formatHourToTime(idleEndHour) + ' 空闲',
            color: '#d1d5db',
          })
        }
      }

      // 如果间隙跨越当前时间，添加未来时间段
      if (slice.startHour > currentHour && lastEndHour < currentHour) {
        segments.push({
          startHour: currentHour,
          endHour: slice.startHour,
          type: 'future',
          label: formatHourToTime(currentHour) + '-' + formatHourToTime(slice.startHour) + ' 未来',
          color: '#f5f5f5',
        })
      }
    }

    // 添加工作时间段
    if (slice.endHour > lastEndHour) {
      const workStartHour = Math.max(slice.startHour, lastEndHour)
      const workEndHour = slice.endHour

      if (workStartHour < currentHour) {
        // 已完成的工作
        const actualEndHour = Math.min(workEndHour, currentHour)
        segments.push({
          startHour: workStartHour,
          endHour: actualEndHour,
          type: 'working',
          efficiency: slice.efficiency,
          label: formatHourToTime(workStartHour) + '-' + formatHourToTime(actualEndHour) + ' 效率' + slice.efficiency + '/5',
          color: getEfficiencyColor(slice.efficiency),
        })
      }

      // 如果工作跨越当前时间，剩余部分算未来
      if (workEndHour > currentHour && slice.startHour < currentHour) {
        segments.push({
          startHour: currentHour,
          endHour: workEndHour,
          type: 'future',
          label: formatHourToTime(currentHour) + '-' + formatHourToTime(workEndHour) + ' 未来',
          color: '#f5f5f5',
        })
      }

      lastEndHour = Math.max(lastEndHour, workEndHour)
    }
  }

  // 添加最后的空闲或未来时间段
  if (lastEndHour < 24) {
    if (lastEndHour < currentHour) {
      // 剩余的空闲时间
      segments.push({
        startHour: lastEndHour,
        endHour: currentHour,
        type: 'idle',
        label: formatHourToTime(lastEndHour) + '-' + formatHourToTime(currentHour) + ' 空闲',
        color: '#d1d5db',
      })
      lastEndHour = currentHour
    }

    // 未来时间
    if (lastEndHour < 24) {
      segments.push({
        startHour: lastEndHour,
        endHour: 24,
        type: 'future',
        label: formatHourToTime(lastEndHour) + '-24:00 未来',
        color: '#f5f5f5',
      })
    }
  }

  return segments
}

// 初始化图表
const initChart = () => {
  if (!chartRef.value) return

  chartInstance = echarts.init(chartRef.value)

  const days = getLast7Days()
  const xAxisData = days.map(d => `${formatWeekday(d)}\n${formatDate(d)}`)

  // 为每一天处理时间段
  const dailySegments = days.map(day => processDayTimeSlices(day))

  // 构建系列数据：需要创建足够多的series来堆叠所有segment
  // 找出最大segment数量
  const maxSegments = Math.max(...dailySegments.map(segments => segments.length))

  // 为每个segment位置创建一个series
  const series: any[] = []
  for (let i = 0; i < maxSegments; i++) {
    const data = dailySegments.map(segments => {
      if (i < segments.length) {
        const segment = segments[i]
        const duration = segment.endHour - segment.startHour
        return {
          value: duration,
          itemStyle: { color: segment.color },
          label: segment.label,
          startHour: segment.startHour,
          endHour: segment.endHour
        }
      }
      return 0 // 没有这个segment就填0
    })

    series.push({
      type: 'bar',
      stack: 'total',
      data: data,
      barWidth: '62.5%',  // 柱子宽度62.5%，间隙37.5%，比例约为1:0.6
      label: {
        show: false
      }
    })
  }

  const option: echarts.EChartsOption = {
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'shadow'
      },
      // 智能定位，避免被其他元素遮挡
      position: function (point, params, dom, rect, size) {
        // point: [x, y] 鼠标位置
        // size: { contentSize: [width, height], viewSize: [width, height] }

        const [mouseX, mouseY] = point
        const [tooltipWidth, tooltipHeight] = size.contentSize
        const [chartWidth, chartHeight] = size.viewSize

        // 默认显示在鼠标右侧
        let x = mouseX + 15
        let y = mouseY - tooltipHeight / 2

        // 如果右侧空间不够，显示在左侧
        if (x + tooltipWidth > chartWidth) {
          x = mouseX - tooltipWidth - 15
        }

        // 确保不超出上边界
        if (y < 0) {
          y = 10
        }

        // 确保不超出下边界
        if (y + tooltipHeight > chartHeight) {
          y = chartHeight - tooltipHeight - 10
        }

        return [x, y]
      },
      formatter: (params: any) => {
        if (!Array.isArray(params) || params.length === 0) return ''

        const dayIndex = params[0].dataIndex
        const day = days[dayIndex]
        const segments = dailySegments[dayIndex]

        // 获取这一天的所有时间片（原始数据）
        const dayStart = new Date(day)
        const dayEnd = new Date(day)
        dayEnd.setDate(dayEnd.getDate() + 1)

        const dayTimeSlices = props.timeSlices.filter(slice => {
          if (!slice.end_at) return false
          const sliceStart = new Date(slice.start_at)
          return sliceStart >= dayStart && sliceStart < dayEnd
        })

        // 计算统计信息（基于原始时间片，而不是处理后的segments）
        const sliceCount = dayTimeSlices.length
        const uniqueTasks = new Set(dayTimeSlices.map(s => s.task_id))
        const taskCount = uniqueTasks.size

        // 计算总工作时长（毫秒转小时）
        const totalWorkMs = dayTimeSlices.reduce((sum, slice) => sum + (slice.duration_ms || 0), 0)
        const totalWorkHours = totalWorkMs / (1000 * 60 * 60)

        // 计算平均效率
        let avgEfficiency = 0
        if (sliceCount > 0) {
          const totalEfficiency = dayTimeSlices.reduce((sum, s) => sum + (s.efficiency_score || 0), 0)
          avgEfficiency = totalEfficiency / sliceCount
        }

        // 计算最长和最短时间片（小时）
        let maxDuration = 0
        let minDuration = Infinity
        dayTimeSlices.forEach(slice => {
          const durationHours = (slice.duration_ms || 0) / (1000 * 60 * 60)
          if (durationHours > maxDuration) maxDuration = durationHours
          if (durationHours < minDuration) minDuration = durationHours
        })

        if (minDuration === Infinity) minDuration = 0

        let content = `<b>${formatWeekday(day)} ${formatDate(day)}</b><br/>`
        content += `<div style="margin-top: 8px;">`
        content += `📊 <b>时间片数目：</b>${sliceCount}<br/>`
        content += `📝 <b>活动任务数：</b>${taskCount}<br/>`
        content += `⏱️ <b>总工作时长：</b>${totalWorkHours.toFixed(1)}小时<br/>`

        if (sliceCount > 0) {
          content += `⭐ <b>整体效率：</b>${avgEfficiency.toFixed(1)}/5<br/>`
          content += `📈 <b>最长时间片：</b>${maxDuration.toFixed(1)}小时<br/>`
          content += `📉 <b>最短时间片：</b>${minDuration.toFixed(1)}小时`
        } else {
          content += `<span style="color: #999;">暂无工作记录</span>`
        }

        content += `</div>`

        return content
      }
    },
    grid: {
      left: '12%',
      right: '2%',
      top: '2%',
      bottom: '2%',
      containLabel: false
    },
    xAxis: {
      type: 'value',
      name: '小时',
      max: 24,
      axisLabel: {
        fontSize: 10,
        color: '#666',
        formatter: (value: number) => value + 'h'
      },
      splitLine: {
        lineStyle: {
          color: '#e5e7eb'
        }
      }
    },
    yAxis: {
      type: 'category',
      data: xAxisData,
      axisLabel: {
        fontSize: 11,
        color: '#666'
      },
      axisTick: {
        show: false
      },
      axisLine: {
        show: false
      }
    },
    series: series
  }

  chartInstance.setOption(option)

  // 添加点击事件
  chartInstance.on('click', (params: any) => {
    if (params.componentType === 'series') {
      const dayIndex = params.dataIndex
      const clickedDay = days[dayIndex]
      if (clickedDay) {
        const year = clickedDay.getFullYear()
        const month = (clickedDay.getMonth() + 1).toString().padStart(2, '0')
        const day = clickedDay.getDate().toString().padStart(2, '0')
        const dateStr = `${year}-${month}-${day}`
        emit('dateClick', dateStr)
      }
    }
  })
}

// 更新图表
const updateChart = () => {
  if (!chartInstance) {
    initChart()
    return
  }

  const days = getLast7Days()
  const xAxisData = days.map(d => `${formatWeekday(d)}\n${formatDate(d)}`)
  const dailySegments = days.map(day => processDayTimeSlices(day))
  const maxSegments = Math.max(...dailySegments.map(segments => segments.length))

  const series: any[] = []
  for (let i = 0; i < maxSegments; i++) {
    const data = dailySegments.map(segments => {
      if (i < segments.length) {
        const segment = segments[i]
        const duration = segment.endHour - segment.startHour
        return {
          value: duration,
          itemStyle: { color: segment.color },
          label: segment.label,
          startHour: segment.startHour,
          endHour: segment.endHour
        }
      }
      return 0
    })

    series.push({
      type: 'bar',
      stack: 'total',
      data: data,
      barWidth: '62.5%',  // 柱子宽度62.5%，间隙37.5%，比例约为1:0.6
      label: {
        show: false
      }
    })
  }

  chartInstance.setOption({
    yAxis: {
      data: xAxisData
    },
    series: series
  })
}

// 监听 props 变化
watch(() => props.timeSlices, () => {
  updateChart()
}, { deep: true })

onMounted(() => {
  initChart()

  // 响应式调整大小
  window.addEventListener('resize', () => {
    chartInstance?.resize()
  })
})
</script>

<template>
  <div ref="chartRef" style="width: 100%; height: 100%"></div>
</template>

<style scoped>
</style>
