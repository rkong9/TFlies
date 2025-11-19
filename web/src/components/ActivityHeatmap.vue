<script setup lang="ts">
import { ref, computed, onMounted, nextTick, watch } from 'vue'
import type { TimeSlice } from '@/types'

interface Props {
  timeSlices: TimeSlice[]
}

const props = defineProps<Props>()

// 定义 emit
const emit = defineEmits<{
  dateClick: [date: string]
}>()

// 选中的年份
const selectedYear = ref(new Date().getFullYear())
const gridScrollRef = ref<HTMLDivElement>()
const yearScrollRef = ref<HTMLDivElement>()

// 颜色方案（类似 GitHub 绿色）
const getColorForLevel = (level: number): string => {
  const colors = [
    '#ebedf0', // 0级 - 无活动（浅灰）
    '#9be9a8', // 1级 - 低活动（浅绿）
    '#40c463', // 2级 - 中等活动（中绿）
    '#30a14e', // 3级 - 高活动（深绿）
    '#216e39', // 4级 - 极高活动（极深绿）
  ]
  return colors[level] || colors[0]
}

// 获取选定年份的所有日期（1月1日到12月31日）
const getAllDaysForYear = (year: number) => {
  const days: Date[] = []
  const startDate = new Date(year, 0, 1) // 1月1日
  const endDate = new Date(year, 11, 31) // 12月31日

  const currentDate = new Date(startDate)
  while (currentDate <= endDate) {
    days.push(new Date(currentDate))
    currentDate.setDate(currentDate.getDate() + 1)
  }

  return days
}

// 格式化日期为 YYYY-MM-DD
const formatDate = (date: Date): string => {
  const year = date.getFullYear()
  const month = (date.getMonth() + 1).toString().padStart(2, '0')
  const day = date.getDate().toString().padStart(2, '0')
  return `${year}-${month}-${day}`
}

// 按日期统计时长（毫秒），正确处理跨天时间片
const getDailyDuration = (): Map<string, number> => {
  const dailyDuration = new Map<string, number>()

  props.timeSlices.forEach(slice => {
    if (!slice.end_at || slice.duration_ms === null) return

    const sliceStart = new Date(slice.start_at)
    const sliceEnd = new Date(slice.end_at)

    // 获取时间片跨越的所有日期
    const currentDate = new Date(sliceStart)
    currentDate.setHours(0, 0, 0, 0)

    while (currentDate < sliceEnd) {
      const dayStart = new Date(currentDate)
      const dayEnd = new Date(currentDate)
      dayEnd.setDate(dayEnd.getDate() + 1)

      // 计算时间片在当前日期的实际工作时长
      const actualStart = Math.max(sliceStart.getTime(), dayStart.getTime())
      const actualEnd = Math.min(sliceEnd.getTime(), dayEnd.getTime())
      const durationInDay = actualEnd - actualStart

      if (durationInDay > 0) {
        const dateKey = formatDate(dayStart)
        const existing = dailyDuration.get(dateKey) || 0
        dailyDuration.set(dateKey, existing + durationInDay)
      }

      // 移动到下一天
      currentDate.setDate(currentDate.getDate() + 1)
    }
  })

  return dailyDuration
}

// 根据时长计算活动等级（0-4）
const getActivityLevel = (durationMs: number): number => {
  if (durationMs === 0) return 0

  const hours = durationMs / (1000 * 60 * 60)

  if (hours < 3) return 1      // 0-3h 少
  if (hours < 6) return 2      // 3-6h 中
  if (hours < 9) return 3      // 6-9h 多
  return 4                     // 9h+ 最多
}

// 获取每个等级的时长范围描述
const getLevelDescription = (level: number): string => {
  const descriptions = [
    '无活动',
    '少 (0-3小时)',
    '中 (3-6小时)',
    '多 (6-9小时)',
    '最多 (9小时以上)'
  ]
  return descriptions[level] || ''
}

// 判断是否是今天
const isToday = (date: Date): boolean => {
  const today = new Date()
  return date.getFullYear() === today.getFullYear() &&
         date.getMonth() === today.getMonth() &&
         date.getDate() === today.getDate()
}

// 格式化时长
const formatDuration = (ms: number): string => {
  const hours = Math.floor(ms / (1000 * 60 * 60))
  const minutes = Math.floor((ms % (1000 * 60 * 60)) / (1000 * 60))

  if (hours > 0) {
    return `${hours}小时${minutes}分钟`
  }
  return `${minutes}分钟`
}

// 构建网格数据（按周组织）
const buildGridData = computed(() => {
  const allDays = getAllDaysForYear(selectedYear.value)
  const dailyDuration = getDailyDuration()

  // 找到第一天是星期几（0=周日, 1=周一, ...）
  const firstDay = allDays[0]
  const firstDayOfWeek = firstDay.getDay()

  // 计算需要多少周
  const totalDays = allDays.length
  const totalWeeks = Math.ceil((totalDays + firstDayOfWeek) / 7)

  // 初始化周数组
  const weeks: Array<Array<{
    date: Date | null
    dateStr: string
    level: number
    duration: number
  }>> = []

  for (let week = 0; week < totalWeeks; week++) {
    const weekDays: typeof weeks[0] = []

    for (let day = 0; day < 7; day++) {
      const dayIndex = week * 7 + day - firstDayOfWeek

      if (dayIndex < 0 || dayIndex >= totalDays) {
        // 填充空白
        weekDays.push({
          date: null,
          dateStr: '',
          level: 0,
          duration: 0
        })
      } else {
        const date = allDays[dayIndex]
        const dateStr = formatDate(date)
        const duration = dailyDuration.get(dateStr) || 0
        const level = getActivityLevel(duration)

        weekDays.push({
          date,
          dateStr,
          level,
          duration
        })
      }
    }

    weeks.push(weekDays)
  }

  return weeks
})

// 获取月份标签（显示在第一周的顶部）
const getMonthLabels = computed(() => {
  const labels: Array<{ month: string; weekIndex: number }> = []
  const weeks = buildGridData.value

  let lastMonth = -1

  weeks.forEach((week, weekIndex) => {
    // 找到这周第一个有效日期
    const firstValidDay = week.find(day => day.date !== null)

    if (firstValidDay && firstValidDay.date) {
      const month = firstValidDay.date.getMonth()

      // 如果月份变化了，添加标签
      if (month !== lastMonth) {
        labels.push({
          month: `${month + 1}月`,
          weekIndex
        })
        lastMonth = month
      }
    }
  })

  return labels
})

// 获取有活动的年份列表
const getActiveYears = computed(() => {
  const years = new Set<number>()

  props.timeSlices.forEach(slice => {
    if (!slice.end_at || slice.duration_ms === null) return
    const year = new Date(slice.start_at).getFullYear()
    years.add(year)
  })

  return Array.from(years).sort((a, b) => a - b)
})

// 星期标签
const weekDayLabels = ['日', '一', '二', '三', '四', '五', '六']

// 悬停状态
const hoveredDay = ref<{ date: Date; duration: number } | null>(null)
const tooltipPosition = ref({ x: 0, y: 0 })

const handleMouseEnter = (day: typeof buildGridData.value[0][0], event: MouseEvent) => {
  if (!day.date) return

  hoveredDay.value = {
    date: day.date,
    duration: day.duration
  }

  // 计算 tooltip 位置
  const rect = (event.target as HTMLElement).getBoundingClientRect()
  tooltipPosition.value = {
    x: rect.left + rect.width / 2,
    y: rect.top - 10
  }
}

const handleMouseLeave = () => {
  hoveredDay.value = null
}

// 点击日期格子
const handleDayClick = (day: typeof buildGridData.value[0][0]) => {
  if (!day.date) return
  emit('dateClick', formatDate(day.date))
}

// 切换年份
const handleYearChange = (year: number) => {
  selectedYear.value = year
}

// 滚动到最右侧
const scrollToRight = async () => {
  await nextTick()
  if (gridScrollRef.value) {
    gridScrollRef.value.scrollLeft = gridScrollRef.value.scrollWidth
  }
  if (yearScrollRef.value) {
    yearScrollRef.value.scrollLeft = yearScrollRef.value.scrollWidth
  }
}

// 监听年份变化，滚动到最右侧
watch(selectedYear, () => {
  scrollToRight()
})

// 组件挂载后滚动到最右侧
onMounted(() => {
  scrollToRight()
})
</script>

<template>
  <div class="activity-heatmap">
    <!-- 图例 -->
    <div class="heatmap-header">
      <div></div>
      <div class="legend">
        <span class="legend-label">少</span>
        <div class="legend-colors">
          <div
            v-for="level in 5"
            :key="level"
            class="legend-block"
            :style="{ backgroundColor: getColorForLevel(level - 1) }"
            :title="getLevelDescription(level - 1)"
          ></div>
        </div>
        <span class="legend-label">多</span>
      </div>
    </div>

    <div class="heatmap-container">
      <!-- 星期标签 -->
      <div class="weekday-labels">
        <div
          v-for="(label, index) in weekDayLabels"
          :key="index"
          class="weekday-label"
          :style="{ visibility: index % 2 === 1 ? 'visible' : 'hidden' }"
        >
          {{ label }}
        </div>
      </div>

      <!-- 可滚动的网格区域 -->
      <div ref="gridScrollRef" class="grid-scroll-container">
        <!-- 月份标签 -->
        <div class="month-labels">
          <div
            v-for="label in getMonthLabels"
            :key="label.weekIndex"
            class="month-label"
            :style="{ left: `${label.weekIndex * 14}px` }"
          >
            {{ label.month }}
          </div>
        </div>

        <!-- 网格 -->
        <div class="grid-container">
          <div
            v-for="(week, weekIndex) in buildGridData"
            :key="weekIndex"
            class="week-column"
          >
            <div
              v-for="(day, dayIndex) in week"
              :key="dayIndex"
              class="day-cell"
              :class="{ 'is-today': day.date && isToday(day.date) }"
              :style="{
                backgroundColor: day.date ? getColorForLevel(day.level) : 'transparent',
                border: day.date && isToday(day.date)
                  ? '2px solid #ef4444'
                  : day.date
                    ? '1px solid rgba(27, 31, 35, 0.06)'
                    : 'none'
              }"
              :title="day.date ? `${formatDate(day.date)}: ${day.duration > 0 ? formatDuration(day.duration) : '无活动'}` : ''"
              @mouseenter="handleMouseEnter(day, $event)"
              @mouseleave="handleMouseLeave"
              @click="handleDayClick(day)"
            ></div>
          </div>
        </div>
      </div>
    </div>

    <!-- 年份按钮组 -->
    <div v-if="getActiveYears.length > 0" ref="yearScrollRef" class="year-buttons-container">
      <button
        v-for="year in getActiveYears"
        :key="year"
        class="year-button"
        :class="{ active: year === selectedYear }"
        @click="handleYearChange(year)"
      >
        {{ year }}
      </button>
    </div>

    <!-- Tooltip -->
    <div
      v-if="hoveredDay"
      class="tooltip"
      :style="{
        left: `${tooltipPosition.x}px`,
        top: `${tooltipPosition.y}px`
      }"
    >
      <div class="tooltip-date">{{ formatDate(hoveredDay.date) }}</div>
      <div class="tooltip-duration">
        {{ hoveredDay.duration > 0 ? formatDuration(hoveredDay.duration) : '无活动' }}
      </div>
    </div>
  </div>
</template>

<style scoped>
.activity-heatmap {
  padding: 16px;
  background: white;
  border-radius: 8px;
}

.heatmap-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.legend {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: #666;
}

.legend-label {
  font-size: 11px;
}

.legend-colors {
  display: flex;
  gap: 3px;
}

.legend-block {
  width: 12px;
  height: 12px;
  border-radius: 2px;
  border: 1px solid rgba(27, 31, 35, 0.06);
  cursor: help;
  transition: all 0.2s ease;
}

.legend-block:hover {
  transform: scale(1.2);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
}

.heatmap-container {
  display: flex;
  gap: 8px;
  margin-bottom: 12px;
}

.weekday-labels {
  display: flex;
  flex-direction: column;
  gap: 3px;
  padding-top: 24px; /* 为月份标签留空间 */
}

.weekday-label {
  height: 11px;
  font-size: 10px;
  color: #666;
  display: flex;
  align-items: center;
  line-height: 11px;
}

.grid-scroll-container {
  flex: 1;
  overflow-x: auto;
  overflow-y: hidden;
  position: relative;
  padding-bottom: 4px; /* 滚动条与内容的间距 */
}

/* 自定义滚动条样式 */
.grid-scroll-container::-webkit-scrollbar {
  height: 8px;
}

.grid-scroll-container::-webkit-scrollbar-track {
  background: #f1f1f1;
  border-radius: 4px;
}

.grid-scroll-container::-webkit-scrollbar-thumb {
  background: #888;
  border-radius: 4px;
}

.grid-scroll-container::-webkit-scrollbar-thumb:hover {
  background: #555;
}

.month-labels {
  position: relative;
  height: 20px;
  margin-bottom: 4px;
}

.month-label {
  position: absolute;
  font-size: 11px;
  color: #666;
  font-weight: 500;
  top: 0;
  white-space: nowrap;
}

.grid-container {
  display: flex;
  gap: 3px;
  padding-bottom: 12px; /* 与滚动条的距离 */
  padding-right: 16px; /* 避免右侧格子被遮挡 */
}

.week-column {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.day-cell {
  width: 11px;
  height: 11px;
  border-radius: 2px;
  cursor: pointer;
  transition: all 0.1s ease;
}

.day-cell:hover {
  outline: 2px solid rgba(27, 31, 35, 0.3);
  outline-offset: 0;
}

/* 年份按钮容器 */
.year-buttons-container {
  display: flex;
  justify-content: flex-end;
  gap: 6px;
  padding: 8px 4px;
  overflow-x: auto;
  overflow-y: hidden;
  border-top: 1px solid #e5e7eb;
  /* 默认隐藏滚动条 */
  scrollbar-width: thin;
  scrollbar-color: transparent transparent;
}

.year-buttons-container::-webkit-scrollbar {
  height: 6px;
}

.year-buttons-container::-webkit-scrollbar-track {
  background: transparent;
}

.year-buttons-container::-webkit-scrollbar-thumb {
  background: transparent;
  border-radius: 3px;
}

/* 鼠标悬停时显示滚动条 */
.year-buttons-container:hover {
  scrollbar-color: #888 #f1f1f1;
}

.year-buttons-container:hover::-webkit-scrollbar-track {
  background: #f1f1f1;
}

.year-buttons-container:hover::-webkit-scrollbar-thumb {
  background: #888;
}

.year-buttons-container:hover::-webkit-scrollbar-thumb:hover {
  background: #555;
}

.year-button {
  flex-shrink: 0;
  padding: 4px 12px;
  font-size: 12px;
  font-weight: 500;
  color: #666;
  background: #f3f4f6;
  border: 1px solid #d1d5db;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
  white-space: nowrap;
}

.year-button:hover {
  background: #e5e7eb;
  border-color: #9ca3af;
}

.year-button.active {
  background: #3b82f6;
  color: white;
  border-color: #3b82f6;
}

.tooltip {
  position: fixed;
  transform: translate(-50%, -100%);
  background: rgba(0, 0, 0, 0.9);
  color: white;
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 12px;
  pointer-events: none;
  z-index: 1000;
  white-space: nowrap;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
}

.tooltip-date {
  font-weight: 600;
  margin-bottom: 4px;
}

.tooltip-duration {
  color: #ddd;
}

.tooltip::after {
  content: '';
  position: absolute;
  bottom: -6px;
  left: 50%;
  transform: translateX(-50%);
  width: 0;
  height: 0;
  border-left: 6px solid transparent;
  border-right: 6px solid transparent;
  border-top: 6px solid rgba(0, 0, 0, 0.9);
}
</style>
