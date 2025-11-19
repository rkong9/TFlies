<script setup lang="ts">
import { NConfigProvider, NMessageProvider, NDialogProvider } from 'naive-ui'
import { useResponsive } from '@/composables/useResponsive'
import MainView from '@/views/MainView.vue'
import MobileView from '@/views/MobileView.vue'

// 设备检测
const { isDesktop, isMobile, deviceType, iPhoneModel } = useResponsive()

// 开发环境检测
const isDev = import.meta.env.DEV

const themeOverrides = {
  common: {
    primaryColor: '#18a058',
    primaryColorHover: '#36ad6a',
    primaryColorPressed: '#0c7a43',
  },
}
</script>

<template>
  <NConfigProvider :theme-overrides="themeOverrides">
    <NMessageProvider>
      <NDialogProvider>
        <!-- 桌面端 -->
        <MainView v-if="isDesktop" />
        
        <!-- 移动端 -->
        <MobileView v-else-if="isMobile" />
        
        <!-- 平板端（暂时使用桌面布局） -->
        <MainView v-else />
        
        <!-- 开发环境设备信息显示 -->
        <div v-if="isDev" class="dev-info">
          {{ deviceType }} | {{ iPhoneModel || 'Non-iPhone' }}
        </div>
      </NDialogProvider>
    </NMessageProvider>
  </NConfigProvider>
</template>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

html, body, #app {
  width: 100%;
  height: 100vh;
  overflow: hidden;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
}

/* iPhone 专用优化 */
html {
  /* iPhone Safari 地址栏问题修复 */
  height: -webkit-fill-available;
}

body {
  /* iPhone Safari 滚动优化 */
  -webkit-overflow-scrolling: touch;
  /* 禁用iPhone Safari默认的触摸高亮 */
  -webkit-tap-highlight-color: transparent;
  -webkit-touch-callout: none;
}

/* 移动端触摸优化 */
@media (max-width: 768px) {
  * {
    /* 禁用双击缩放 */
    touch-action: manipulation;
  }
  
  /* 输入框例外 */
  input, textarea, select {
    touch-action: auto;
  }
}

/* 开发环境设备信息 */
.dev-info {
  position: fixed;
  top: 10px;
  right: 10px;
  background: rgba(0, 0, 0, 0.8);
  color: white;
  padding: 4px 8px;
  font-size: 12px;
  border-radius: 4px;
  z-index: 9999;
  pointer-events: none;
}
</style>
