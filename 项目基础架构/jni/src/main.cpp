/**
 * ============================================================================
 * 王者荣耀 - 简单位置读取示例
 * ============================================================================
 *
 * 【项目目的】
 * 这是一个教学示例,展示用户程序如何通过内核驱动读取游戏进程内存
 * 具体功能: 读取王者荣耀中英雄的坐标位置
 *
 * 【完整交互流程图】
 *
 *   main.cpp (用户程序)
 *        │
 *        ├─1. new Driver()
 *        │      │
 *        │      └─ open("/dev/xxx", O_RDWR)  ──────────┐
 *        │                                              │
 *        ├─2. driver->initialize("com.tencent.tmgp.sgame")
 *        │      │                                       │
 *        │      └─ popen("pidof xxx") 获取进程ID        │
 *        │                                              ▼
 *        ├─3. getBssAddress(pid, "libGameCore.so")   ┌─────────────┐
 *        │      │                                    │ 内核驱动     │
 *        │      └─ 读取/proc/[pid]/maps              │ (xxx.ko)    │
 *        │                                           │             │
 *        ├─4. driver->read<uint64_t>(bss + 偏移)     │  ioctl处理  │
 *        │      │                                    │      ↓      │
 *        │      ├─ 填充COPY_MEMORY结构体             │ 读取目标    │
 *        │      │                                    │ 进程内存    │
 *        │      ├─ ioctl(fd, 0x801, &cm) ───────────►│      ↓      │
 *        │      │                                    │ 返回数据    │
 *        │      └─ 接收驱动返回的数据 ◄─────────────┼─────────────┘
 *        │
 *        └─5. printf() 打印坐标
 *
 *
 * 【偏移说明】
 * 偏移是游戏逆向分析得出的,不同游戏版本偏移可能不同
 * 这里使用的是王者荣耀国际服的偏移:
 *
 * 英雄数组入口: libGameCore.so:bss + 0x158918 + 0x238
 * 英雄坐标: 英雄对象 + 0x248 + 0x10 + 0x0 + 0x10 + 0x0 (X坐标)
 *                                                  + 0x8 (Y坐标)
 *
 * 【指针链解析图示】
 *
 *   bss基址
 *      │
 *      ├─ +0x158918 ──► [指针1]
 *      │                    │
 *      │                    └─ +0x238 ──► [英雄数组基址]
 *      │                                       │
 *      │                                       └─ +i*0x18 ──► [第i个英雄对象]
 *      │                                                            │
 *      │   ┌────────────────────────────────────────────────────────┘
 *      │   │
 *      │   ├─ +0x30  ──► 英雄ID
 *      │   ├─ +0x3C  ──► 阵营 (1=蓝方, 2=红方)
 *      │   ├─ +0x168 ──► [血量结构体指针]
 *      │   │                  ├─ +0x98 ──► 当前血量
 *      │   │                  └─ +0xA0 ──► 最大血量
 *      │   │
 *      │   └─ +0x248 ──► [坐标结构体指针1]
 *      │                      │
 *      │                      └─ +0x10 ──► [坐标结构体指针2]
 *      │                                       │
 *      │                                       └─ +0x0 ──► [坐标结构体指针3]
 *      │                                                       │
 *      │                                                       └─ +0x10 ──► [最终坐标]
 *      │                                                                        │
 *      │                                                                        ├─ +0x0 ──► X坐标 (int)
 *      │                                                                        └─ +0x8 ──► Y坐标 (int)
 *
 */

#include <stdio.h>
#include <unistd.h>
#include "driver.h"

// 全局驱动对象
Driver* driver = nullptr;

// 游戏相关基址
uintptr_t libGameCore_bss = 0;   // libGameCore.so 的 bss 段地址
uintptr_t libil2cpp_bss = 0;     // libil2cpp.so 的 bss 段地址


/**
 * ============================================================================
 * 英雄数据结构定义
 * ============================================================================
 */
struct HeroInfo {
    int id;             // 英雄ID
    int team;           // 阵营 (1=蓝方, 2=红方)
    int hp;             // 当前血量
    int maxHp;          // 最大血量
    int posX;           // X坐标
    int posY;           // Y坐标
};


/**
 * ============================================================================
 * 读取指针 - 链式解引用的辅助函数
 * ============================================================================
 *
 * 游戏中的数据通常需要通过多级指针才能访问到
 * 这个函数简化了读取指针的操作
 */
uintptr_t readPointer(uintptr_t addr) {
    if (addr == 0) return 0;

    // 地址有效性检查
    addr = addr & 0xFFFFFFFFFFFF;

    uintptr_t value = driver->read<uint64_t>(addr);
    return value & 0xFFFFFFFFFFFF;  // 48位地址掩码
}


/**
 * ============================================================================
 * 读取单个英雄的信息
 * ============================================================================
 *
 * 【读取流程详解】
 *
 * 1. 从bss基址开始,按照偏移链逐级读取指针
 * 2. 最终到达英雄对象的内存位置
 * 3. 从英雄对象读取各个属性(ID、血量、坐标等)
 *
 * @param heroArrayBase 英雄数组的基址
 * @param index 英雄索引 (0-9)
 * @param info 输出的英雄信息
 * @return 成功返回true
 */
bool readHeroInfo(uintptr_t heroArray, int index, HeroInfo& info) {
    // 正确的读取方式 (参考王者简单源码):
    // heroPtr = read(read(heroArrayPtr) + i * 0x18)
    // 但 heroArray 已经是 read(heroArrayPtr) 的结果了
    // 所以这里直接: heroPtr = read(heroArray + i * 0x18)
    uintptr_t heroPtr = readPointer(heroArray + index * 0x18);
    if (heroPtr == 0) {
        return false;
    }

    // 读取英雄ID
    // 偏移 +0x30
    info.id = driver->read<int>(heroPtr + 0x30);

    // 读取阵营
    // 偏移 +0x3C
    // 1 = 蓝方, 2 = 红方
    info.team = driver->read<int>(heroPtr + 0x3C);

    // 读取血量
    // 偏移链: +0x168 -> +0x98 (当前血量), +0xA0 (最大血量)
    uintptr_t hpStructPtr = readPointer(heroPtr + 0x168);
    if (hpStructPtr != 0) {
        info.hp = driver->read<int>(hpStructPtr + 0x98);
        info.maxHp = driver->read<int>(hpStructPtr + 0xA0);
    }

    // 读取坐标
    // 偏移链: +0x248 -> +0x10 -> +0x0 -> +0x10 -> +0x0(X) / +0x8(Y)
    //
    // 【为什么这么复杂?】
    // Unity游戏的对象结构是嵌套的:
    // - 英雄对象包含Transform组件 (+0x248)
    // - Transform包含位置信息,但可能有多层封装
    // - 最终的坐标存储在最内层的结构中
    uintptr_t ptr1 = readPointer(heroPtr + 0x248);
    if (ptr1 == 0) return false;

    uintptr_t ptr2 = readPointer(ptr1 + 0x10);
    if (ptr2 == 0) return false;

    uintptr_t ptr3 = readPointer(ptr2 + 0x0);
    if (ptr3 == 0) return false;

    uintptr_t coordPtr = readPointer(ptr3 + 0x10);
    if (coordPtr == 0) return false;

    // 最终读取X和Y坐标
    info.posX = driver->read<int>(coordPtr + 0x0);
    info.posY = driver->read<int>(coordPtr + 0x8);

    return true;
}


/**
 * ============================================================================
 * 初始化 - 连接驱动并获取基址
 * ============================================================================
 */
bool initialize() {
    printf("========================================\n");
    printf("  王者荣耀位置读取示例 - 教学版\n");
    printf("========================================\n\n");

    // 步骤1: 创建驱动对象 (内部会自动查找并打开驱动设备)
    printf("[步骤1] 连接内核驱动...\n");
    driver = new Driver();
    if (!driver->isValid()) {
        printf("[-] 驱动连接失败! 请确保驱动已加载\n");
        return false;
    }

    // 步骤2: 获取游戏进程ID
    printf("\n[步骤2] 获取游戏进程...\n");
    // 国服: com.tencent.tmgp.sgame
    // 国际服: com.levelinfinite.sgameGlobal
    pid_t pid = driver->initialize("com.tencent.tmgp.sgame");
    if (pid == 0) {
        // 尝试国际服
        pid = driver->initialize("com.levelinfinite.sgameGlobal");
    }
    if (pid == 0) {
        printf("[-] 未找到王者荣耀进程! 请先启动游戏\n");
        return false;
    }

    // 步骤3: 获取模块bss段地址
    printf("\n[步骤3] 获取模块基址...\n");
    libGameCore_bss = getBssAddress(pid, "libGameCore.so");
    if (libGameCore_bss == 0) {
        printf("[-] 获取libGameCore.so基址失败!\n");
        return false;
    }

    libil2cpp_bss = getBssAddress(pid, "libil2cpp.so");
    // libil2cpp可选,不影响基本功能

    printf("\n[初始化完成]\n");
    printf("  进程ID: %d\n", pid);
    printf("  libGameCore.so bss: 0x%lx\n", libGameCore_bss);
    printf("  libil2cpp.so bss: 0x%lx\n", libil2cpp_bss);
    printf("========================================\n\n");

    return true;
}


/**
 * ============================================================================
 * 读取所有英雄位置
 * ============================================================================
 *
 * 【内存读取流程详解】
 *
 * 1. 从 libGameCore.so 的 bss 段开始
 *
 * 2. 第一级指针读取:
 *    temp1 = read(bss + 0x158918)  // 获取游戏对象管理器
 *
 * 3. 第二级获取英雄数组基址:
 *    heroArrayBase = temp1 + 0x238  // 英雄数组偏移
 *    heroArray = read(heroArrayBase) // 读取数组指针
 *
 * 4. 遍历英雄数组 (0-9, 共10个英雄):
 *    for i in 0..9:
 *        heroPtr = read(heroArray + i * 0x18)  // 每个元素0x18字节
 *        // 从heroPtr读取英雄数据...
 */
void readAllHeroPositions() {
    printf("\n【读取英雄位置】\n");
    printf("----------------------------------------\n");

    // ===============================================
    // 第一步: 获取英雄数组的入口地址
    // ===============================================
    //
    // 偏移说明:
    // libGameCore.so:bss + 0x158918 是游戏对象管理器的指针
    // +0x238 是英雄数组在管理器中的偏移

    uintptr_t temp1 = readPointer(libGameCore_bss + 0x158918);
    if (temp1 == 0) {
        printf("[-] 读取游戏对象管理器失败,可能不在对局中\n");
        return;
    }
    printf("[调试] 游戏对象管理器: 0x%lx\n", temp1);

    uintptr_t heroArrayPtr = temp1 + 0x238;
    uintptr_t heroArray = readPointer(heroArrayPtr);
    if (heroArray == 0) {
        printf("[-] 读取英雄数组指针失败\n");
        return;
    }
    printf("[调试] 英雄数组指针: 0x%lx\n", heroArray);

    // ===============================================
    // 第二步: 遍历读取每个英雄的信息
    // ===============================================
    //
    // 王者荣耀每局10个英雄 (5v5)
    // 英雄数组中按顺序存储:
    // 0-4: 一方阵营的5个英雄
    // 5-9: 另一方阵营的5个英雄

    printf("\n%-6s %-8s %-12s %-15s\n", "索引", "阵营", "血量", "坐标");
    printf("----------------------------------------\n");

    for (int i = 0; i < 10; i++) {
        HeroInfo hero = {};

        if (readHeroInfo(heroArray, i, hero)) {
            // 过滤无效数据
            if (hero.id > 0 && hero.maxHp > 0) {
                const char* teamStr = (hero.team == 1) ? "蓝方" : "红方";
                printf("%-6d %-8s %5d/%-5d  (%d, %d)\n",
                       i,
                       teamStr,
                       hero.hp, hero.maxHp,
                       hero.posX, hero.posY);
            }
        }
    }

    printf("----------------------------------------\n");
}


/**
 * ============================================================================
 * 主函数
 * ============================================================================
 */
int main() {
    // 初始化
    if (!initialize()) {
        printf("\n[-] 初始化失败,程序退出\n");
        return -1;
    }

    // 主循环 - 持续读取并显示英雄位置
    printf("\n开始读取英雄位置 (每2秒刷新一次, Ctrl+C 退出)...\n");

    while (true) {
        readAllHeroPositions();

        // 等待2秒后再次读取
        sleep(2);

        // 清屏效果 (可选)
        printf("\033[2J\033[H");  // ANSI清屏
    }

    // 清理
    delete driver;
    return 0;
}
