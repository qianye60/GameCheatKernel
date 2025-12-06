# 王者荣耀内存读取示例 - 教学项目

仅供学习内存读取原理，研究游戏逆向技术。

## 目录结构

```
项目基础架构/
├── jni/                        # NDK 源码目录
│   ├── Android.mk              # 编译配置
│   ├── Application.mk          # 应用配置
│   ├── include/                # 头文件
│   │   └── driver.h            # 驱动通信封装
│   └── src/                    # 源代码
│       ├── main.cpp            # 简单示例
│       └── wzry_reader.cpp     # 完整王者读取示例
├── docs/                       # 文档
│   └── 偏移分析教程.md
├── libs/                       # 编译输出 (自动生成)
│   └── arm64-v8a/
│       └── QianyeKernel
├── build.bat                   # Windows 编译脚本
├── pack.ps1                    # 打包脚本
└── QianyeKernel.sh            # 打包后的自解压脚本
```

## 编译方法

### Windows

1. 安装 Android NDK (推荐 r27d)
2. 修改 `build.bat` 中的 NDK 路径
3. 双击运行 `build.bat`

### 手动编译

```bash
cd 项目基础架构
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk
```

## 使用方法

```bash
# 推送到手机
adb push QianyeKernel.sh /data/local/tmp/

# 执行 (需要 root + 已加载驱动)
adb shell su -c "sh /data/local/tmp/QianyeKernel.sh"
```

## 偏移说明

偏移来自公开源码分析，可能已过期。详见 `docs/偏移分析教程.md`

主要偏移链：
```
libGameCore.so + 0x452E5C8 → 游戏管理器
    +0x120 → 英雄列表
        +i*0x18 → +0x68 → 英雄对象
            +0x30 → 英雄ID
            +0x3C → 阵营
            +0x168 → 血量结构
            +0x248 → 坐标指针链
```

## 注意事项

- 需要 root 权限
- 需要先加载内核驱动
- 偏移可能随游戏更新失效
- 仅供学习研究使用
