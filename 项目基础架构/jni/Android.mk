# ============================================================================
# Android.mk - NDK 编译配置文件
# ============================================================================
#
# 【编译方法】
# 在项目根目录执行: ndk-build
# 或使用: build.bat
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# 模块名称
LOCAL_MODULE := QianyeKernel

# 源文件 (相对于 jni 目录)
# 可选: src/main.cpp (原始示例) 或 src/wzry_reader.cpp (完整示例)
LOCAL_SRC_FILES := src/wzry_reader.cpp

# 头文件目录
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

# 编译选项
LOCAL_CFLAGS := -w -s -O2
LOCAL_CPPFLAGS := -std=c++17

# 链接库
LOCAL_LDLIBS := -llog

# 生成可执行文件
include $(BUILD_EXECUTABLE)
