# ============================================================================
# Application.mk - NDK 应用级配置
# ============================================================================
#
# 【配置说明】
#
# APP_ABI: 目标CPU架构
#   - arm64-v8a: 64位ARM (现代手机都是这个)
#   - armeabi-v7a: 32位ARM (老设备)
#   - x86_64: 64位x86 (模拟器)
#
# APP_PLATFORM: 最低支持的Android API级别
#   - android-21 对应 Android 5.0
#
# APP_STL: C++标准库实现
#   - c++_static: 静态链接libc++ (推荐,可执行文件独立)
#   - c++_shared: 动态链接libc++
#

# 目标架构 - 64位ARM
APP_ABI := arm64-v8a

# 最低API级别
APP_PLATFORM := android-21

# C++标准库 - 静态链接
APP_STL := c++_static

# 优化级别
APP_OPTIM := release
