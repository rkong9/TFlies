import { ref, computed, onMounted, onUnmounted } from 'vue'

export type DeviceType = 'desktop' | 'mobile' | 'tablet'
export type MobileDevice = 'iPhone' | 'android' | 'unknown'

export const useResponsive = () => {
  const windowWidth = ref(0)
  const windowHeight = ref(0)
  const userAgent = ref('')

  // 基础设备类型
  const deviceType = computed<DeviceType>(() => {
    if (windowWidth.value < 768) return 'mobile'
    if (windowWidth.value < 1024) return 'tablet'
    return 'desktop'
  })

  // 移动设备检测
  const mobileDevice = computed<MobileDevice>(() => {
    if (/iPhone/i.test(userAgent.value)) return 'iPhone'
    if (/Android/i.test(userAgent.value)) return 'android'
    return 'unknown'
  })

  // iPhone 型号检测
  const iPhoneModel = computed(() => {
    if (mobileDevice.value !== 'iPhone') return null

    const width = Math.min(windowWidth.value, windowHeight.value)
    const height = Math.max(windowWidth.value, windowHeight.value)

    // 基于屏幕尺寸判断iPhone型号
    if (width === 375 && height === 667) return 'iPhone SE (3rd gen)'
    if (width === 390 && height === 844) return 'iPhone 14'
    if (width === 393 && height === 852) return 'iPhone 14 Pro'
    if (width === 414 && height === 896) return 'iPhone 15'
    if (width === 393 && height === 852) return 'iPhone 15 Pro'
    if (width === 428 && height === 926) return 'iPhone 14 Plus'
    if (width === 430 && height === 932) return 'iPhone 14 Pro Max'
    
    return 'iPhone (Unknown Model)'
  })

  // Safari 检测
  const isSafari = computed(() => {
    return /Safari/i.test(userAgent.value) && !/Chrome/i.test(userAgent.value)
  })

  // Safe Area 支持检测
  const supportsSafeArea = computed(() => {
    return CSS.supports('padding-top: env(safe-area-inset-top)')
  })

  // 设备特性
  const isDesktop = computed(() => deviceType.value === 'desktop')
  const isMobile = computed(() => deviceType.value === 'mobile')
  const isTablet = computed(() => deviceType.value === 'tablet')
  const isIPhone = computed(() => mobileDevice.value === 'iPhone')
  const isAndroid = computed(() => mobileDevice.value === 'android')

  // 更新尺寸信息
  const updateDimensions = () => {
    windowWidth.value = window.innerWidth
    windowHeight.value = window.innerHeight
    userAgent.value = navigator.userAgent

    // 开发环境调试信息
    if (import.meta.env.DEV) {
      console.log('📱 设备检测:', {
        deviceType: deviceType.value,
        mobileDevice: mobileDevice.value,
        iPhoneModel: iPhoneModel.value,
        screenSize: `${windowWidth.value}x${windowHeight.value}`,
        isSafari: isSafari.value,
        supportsSafeArea: supportsSafeArea.value
      })
    }
  }

  // 生命周期
  onMounted(() => {
    updateDimensions()
    window.addEventListener('resize', updateDimensions)
    window.addEventListener('orientationchange', updateDimensions)
  })

  onUnmounted(() => {
    window.removeEventListener('resize', updateDimensions)
    window.removeEventListener('orientationchange', updateDimensions)
  })

  return {
    // 尺寸信息
    windowWidth: computed(() => windowWidth.value),
    windowHeight: computed(() => windowHeight.value),
    
    // 设备类型
    deviceType,
    mobileDevice,
    iPhoneModel,
    
    // 浏览器信息
    isSafari,
    supportsSafeArea,
    
    // 便捷判断
    isDesktop,
    isMobile,
    isTablet,
    isIPhone,
    isAndroid,
    
    // 方法
    updateDimensions
  }
}