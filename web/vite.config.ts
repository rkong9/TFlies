import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  server: {
    host: '0.0.0.0', // 允许局域网访问，iPhone 15 Pro可以访问
    port: 3000,
    strictPort: true, // 严格使用3000端口
    cors: true, // 允许跨域请求
    // 如果需要HTTPS访问（某些iPhone功能需要）
    // https: true,
    proxy: {
      '/api': {
        target: 'http://localhost:8000',
        changeOrigin: true,
      }
    }
  },
  // 移动端调试优化
  define: {
    __DEV_MOBILE__: process.env.NODE_ENV === 'development'
  }
})
