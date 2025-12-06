@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ============================================
echo   编译 + 打包脚本
echo ============================================
echo.

:: NDK 路径配置
set NDK_PATH=E:\android-ndk-r27d

:: 检查 NDK
if not exist "%NDK_PATH%\ndk-build.cmd" (
    echo [错误] 未找到 NDK: %NDK_PATH%
    pause
    exit /b 1
)
echo [√] NDK: %NDK_PATH%

:: 编译 (jni 目录结构)
echo.
echo [1/2] 编译中...
call "%NDK_PATH%\ndk-build.cmd" NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk

if %errorlevel% neq 0 (
    echo [错误] 编译失败!
    pause
    exit /b 1
)

:: 打包成 .sh
echo.
echo [2/2] 打包成 .sh...
call powershell -ExecutionPolicy Bypass -File pack.ps1

echo.
echo ============================================
echo   完成!
echo ============================================
echo.
echo 输出文件:
echo   - libs\arm64-v8a\QianyeKernel  (原始二进制)
echo   - QianyeKernel.sh              (自解压脚本)
echo.
echo 使用方法:
echo   adb push QianyeKernel.sh /data/local/tmp/
echo   adb shell su -c "sh /data/local/tmp/QianyeKernel.sh"
echo.
pause
