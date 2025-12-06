/**
 * ============================================================================
 * 王者荣耀内存读取示例 - 教学版
 * ============================================================================
 *
 * 基于天理源码的偏移结构分析
 * 仅供学习内存读取原理，偏移可能已过期
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "driver.h"

// ============================================================================
// 全局变量
// ============================================================================

Driver* driver = nullptr;
pid_t gamePid = 0;

// 基址
uintptr_t libGameCore_base = 0;    // libGameCore.so 模块基址
uintptr_t libGameCore_bss = 0;     // libGameCore.so 的 bss 段
uintptr_t libil2cpp_bss = 0;       // libil2cpp.so 的 bss 段

// ============================================================================
// 偏移定义 (2024年12月实测有效)
// ============================================================================

namespace Offset {
    // 第一级偏移 - 从 BSS 段开始 (重要！不是从 base 开始)
    const uintptr_t GameManager = 0x2728;        // BSS + 0x2728 = 游戏管理器指针

    // 游戏管理器内部偏移
    const uintptr_t HeroList = 0x8;              // +0x8 -> 英雄列表

    // 英雄数组
    const uintptr_t HeroInterval = 0x18;         // 每个英雄间隔
    const uintptr_t HeroObject = 0x68;           // +0x68 -> 英雄对象

    // 英雄对象内部偏移
    const uintptr_t HeroId = 0x30;               // 英雄ID
    const uintptr_t HeroCamp = 0x3C;             // 阵营 (1=蓝方, 2=红方)
    const uintptr_t HpStruct = 0x168;            // 血量结构体指针
    const uintptr_t CoordPtr1 = 0x248;           // 坐标指针链第1级

    // 血量结构体偏移
    const uintptr_t CurrentHp = 0xa8;            // 当前血量
    const uintptr_t MaxHp = 0xb0;                // 最大血量

    // 坐标指针链
    const uintptr_t CoordPtr2 = 0x10;            // 坐标指针链第2级
    const uintptr_t CoordPtr3 = 0x0;             // 坐标指针链第3级
    const uintptr_t CoordPtr4 = 0x60;            // 坐标指针链第4级
    const uintptr_t CoordX = 0x0;                // X坐标
    const uintptr_t CoordY = 0x8;                // Y坐标
}

// ============================================================================
// 英雄数据结构
// ============================================================================

struct HeroData {
    int index;          // 索引 (0-9)
    int id;             // 英雄ID
    int camp;           // 阵营
    int currentHp;      // 当前血量
    int maxHp;          // 最大血量
    int posX;           // X坐标
    int posY;           // Y坐标
    bool isValid;       // 数据是否有效
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * 读取指针 (带地址有效性检查)
 */
uintptr_t ReadPointer(uintptr_t addr) {
    if (addr == 0) return 0;

    // 地址掩码 (48位)
    addr = addr & 0xFFFFFFFFFFFF;

    // 简单的地址有效性检查
    if (addr < 0x1000000000 || addr > 0xFFFFFFFFFFFF) {
        return 0;
    }

    uintptr_t value = driver->read<uint64_t>(addr);
    return value & 0xFFFFFFFFFFFF;
}

/**
 * 读取整数
 */
int ReadInt(uintptr_t addr) {
    if (addr == 0) return 0;
    return driver->read<int>(addr);
}

/**
 * 打印分隔线
 */
void PrintLine() {
    printf("────────────────────────────────────────────────────────\n");
}

// ============================================================================
// 核心读取函数
// ============================================================================

/**
 * 初始化 - 连接驱动，获取基址
 */
bool Initialize() {
    printf("\n");
    PrintLine();
    printf("  王者荣耀内存读取示例 - 教学版\n");
    PrintLine();

    // 1. 创建驱动对象
    printf("\n[步骤1] 连接内核驱动...\n");
    driver = new Driver();
    if (!driver->isValid()) {
        printf("[-] 驱动连接失败！请确保已加载驱动\n");
        return false;
    }
    printf("[+] 驱动连接成功\n");

    // 2. 获取游戏进程
    printf("\n[步骤2] 获取游戏进程...\n");
    gamePid = driver->initialize("com.tencent.tmgp.sgame");
    if (gamePid == 0) {
        printf("[-] 未找到王者荣耀进程！请先启动游戏\n");
        return false;
    }
    printf("[+] 游戏PID: %d\n", gamePid);

    // 3. 获取模块基址
    printf("\n[步骤3] 获取模块基址...\n");

    // 方法1: 通过驱动获取
    libGameCore_base = driver->getModuleBase("libGameCore.so");

    // 方法2: 通过 /proc/pid/maps 获取 bss 段
    libGameCore_bss = getBssAddress(gamePid, "libGameCore.so");
    libil2cpp_bss = getBssAddress(gamePid, "libil2cpp.so");

    printf("[+] libGameCore.so 基址: 0x%lx\n", libGameCore_base);
    printf("[+] libGameCore.so BSS:  0x%lx\n", libGameCore_bss);
    printf("[+] libil2cpp.so BSS:    0x%lx\n", libil2cpp_bss);

    if (libGameCore_base == 0 && libGameCore_bss == 0) {
        printf("[-] 获取基址失败！\n");
        return false;
    }

    PrintLine();
    printf("[+] 初始化完成\n");
    PrintLine();

    return true;
}

/**
 * 读取单个英雄的坐标
 *
 * 偏移链：
 * heroPtr + 0x248 -> +0x10 -> +0x0 -> +0x60 -> X(+0x0) / Y(+0x8)
 */
bool ReadHeroCoord(uintptr_t heroPtr, int& outX, int& outY) {
    if (heroPtr == 0) return false;

    // 第1级: heroPtr + 0x248
    uintptr_t ptr1 = ReadPointer(heroPtr + Offset::CoordPtr1);
    if (ptr1 == 0) return false;

    // 第2级: ptr1 + 0x10
    uintptr_t ptr2 = ReadPointer(ptr1 + Offset::CoordPtr2);
    if (ptr2 == 0) return false;

    // 第3级: ptr2 + 0x0 (直接读取)
    uintptr_t ptr3 = ReadPointer(ptr2 + Offset::CoordPtr3);
    if (ptr3 == 0) return false;

    // 第4级: ptr3 + 0x60
    uintptr_t coordPtr = ReadPointer(ptr3 + Offset::CoordPtr4);
    if (coordPtr == 0) return false;

    // 读取坐标
    outX = ReadInt(coordPtr + Offset::CoordX);
    outY = ReadInt(coordPtr + Offset::CoordY);

    return true;
}

/**
 * 读取单个英雄的完整数据
 */
bool ReadHeroData(uintptr_t heroPtr, int index, HeroData& data) {
    data.index = index;
    data.isValid = false;

    if (heroPtr == 0) return false;

    // 读取英雄ID
    data.id = ReadInt(heroPtr + Offset::HeroId);
    if (data.id <= 0 || data.id > 1000) return false;  // ID范围检查

    // 读取阵营
    data.camp = ReadInt(heroPtr + Offset::HeroCamp);
    if (data.camp != 1 && data.camp != 2) return false;  // 阵营只能是1或2

    // 读取血量
    uintptr_t hpStruct = ReadPointer(heroPtr + Offset::HpStruct);
    if (hpStruct != 0) {
        data.currentHp = ReadInt(hpStruct + Offset::CurrentHp);
        data.maxHp = ReadInt(hpStruct + Offset::MaxHp);
    }

    // 读取坐标
    ReadHeroCoord(heroPtr, data.posX, data.posY);

    // 验证数据有效性
    if (data.maxHp > 0 && data.posX != 0 && data.posY != 0) {
        data.isValid = true;
    }

    return data.isValid;
}

/**
 * 读取所有英雄数据
 *
 * 偏移链：
 * BSS + 0x2728 -> 游戏管理器
 * 游戏管理器 + 0x8 -> 英雄列表
 * 英雄列表 + i*0x18 -> +0x68 -> 英雄对象
 */
void ReadAllHeroes() {
    printf("\n[读取英雄数据]\n");
    PrintLine();

    // 第1步: 获取游戏管理器 (从 BSS 开始!)
    uintptr_t gameManager = ReadPointer(libGameCore_bss + Offset::GameManager);
    if (gameManager == 0) {
        printf("[-] 读取游戏管理器失败，可能不在对局中\n");
        return;
    }
    printf("[调试] 游戏管理器地址: 0x%lx\n", gameManager);

    // 第2步: 获取英雄列表
    uintptr_t heroList = ReadPointer(gameManager + Offset::HeroList);
    if (heroList == 0) {
        printf("[-] 英雄列表为空\n");
        return;
    }
    printf("[调试] 英雄列表地址: 0x%lx\n", heroList);

    PrintLine();
    printf("%-4s %-6s %-6s %-12s %-20s\n", "序号", "阵营", "英雄ID", "血量", "坐标");
    PrintLine();

    // 第3步: 遍历英雄数组
    int validCount = 0;
    int selfCamp = 0;

    for (int i = 0; i < 10; i++) {
        // 计算英雄对象地址
        uintptr_t heroArrayPtr = ReadPointer(heroList + i * Offset::HeroInterval);
        uintptr_t heroPtr = ReadPointer(heroArrayPtr + Offset::HeroObject);

        if (heroPtr == 0) continue;

        // 读取英雄数据
        HeroData hero;
        if (ReadHeroData(heroPtr, i, hero)) {
            // 第一个蓝方英雄作为自己
            if (selfCamp == 0 && hero.camp == 1) {
                selfCamp = 1;
            }

            const char* campStr = (hero.camp == 1) ? "蓝方" : "红方";
            const char* selfMark = (hero.camp != selfCamp && selfCamp != 0) ? " [敌]" : "";

            printf("%-4d %-6s %-6d %5d/%-5d  (%d, %d)%s\n",
                   hero.index,
                   campStr,
                   hero.id,
                   hero.currentHp, hero.maxHp,
                   hero.posX, hero.posY,
                   selfMark);
            validCount++;
        }
    }

    PrintLine();
    printf("共读取到 %d 个有效英雄\n", validCount);
}

/**
 * 调试模式 - 打印偏移链每一步的值
 */
void DebugOffsetChain() {
    printf("\n[调试偏移链]\n");
    PrintLine();

    printf("libGameCore.so BSS:     0x%lx\n", libGameCore_bss);
    printf("偏移 GameManager:       0x%lx\n", Offset::GameManager);
    printf("计算地址:               0x%lx\n", libGameCore_bss + Offset::GameManager);

    uintptr_t gameManager = ReadPointer(libGameCore_bss + Offset::GameManager);
    printf("\n读取游戏管理器: 0x%lx\n", gameManager);

    if (gameManager == 0) {
        printf("[-] 游戏管理器为空，可能不在对局中\n");
        return;
    }

    printf("\n英雄列表:\n");
    uintptr_t heroList = ReadPointer(gameManager + Offset::HeroList);
    printf("  heroList = read(gameManager + 0x%lx) = 0x%lx\n", Offset::HeroList, heroList);

    // 读取第一个有效英雄
    for (int i = 0; i < 10; i++) {
        uintptr_t heroArrayPtr = ReadPointer(heroList + i * Offset::HeroInterval);
        if (heroArrayPtr == 0) continue;

        uintptr_t heroPtr = ReadPointer(heroArrayPtr + Offset::HeroObject);
        if (heroPtr == 0) continue;

        int heroId = ReadInt(heroPtr + Offset::HeroId);
        int camp = ReadInt(heroPtr + Offset::HeroCamp);

        if (heroId > 0 && heroId < 1000 && (camp == 1 || camp == 2)) {
            printf("\n找到英雄 [%d]:\n", i);
            printf("  heroArrayPtr = read(heroList + %d*0x%lx) = 0x%lx\n", i, Offset::HeroInterval, heroArrayPtr);
            printf("  heroPtr = read(heroArrayPtr + 0x%lx) = 0x%lx\n", Offset::HeroObject, heroPtr);
            printf("  英雄ID = %d\n", heroId);
            printf("  阵营   = %d (%s)\n", camp, camp == 1 ? "蓝方" : "红方");

            printf("\n坐标指针链:\n");
            uintptr_t ptr1 = ReadPointer(heroPtr + Offset::CoordPtr1);
            printf("  ptr1 = read(heroPtr + 0x%lx) = 0x%lx\n", Offset::CoordPtr1, ptr1);

            uintptr_t ptr2 = ReadPointer(ptr1 + Offset::CoordPtr2);
            printf("  ptr2 = read(ptr1 + 0x%lx) = 0x%lx\n", Offset::CoordPtr2, ptr2);

            uintptr_t ptr3 = ReadPointer(ptr2 + Offset::CoordPtr3);
            printf("  ptr3 = read(ptr2 + 0x%lx) = 0x%lx\n", Offset::CoordPtr3, ptr3);

            uintptr_t coordPtr = ReadPointer(ptr3 + Offset::CoordPtr4);
            printf("  coordPtr = read(ptr3 + 0x%lx) = 0x%lx\n", Offset::CoordPtr4, coordPtr);

            if (coordPtr != 0) {
                int x = ReadInt(coordPtr + Offset::CoordX);
                int y = ReadInt(coordPtr + Offset::CoordY);
                printf("\n最终坐标: X=%d, Y=%d\n", x, y);
            }

            break;
        }
    }

    PrintLine();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    // 初始化
    if (!Initialize()) {
        printf("\n[-] 初始化失败，程序退出\n");
        return -1;
    }

    // 菜单
    while (true) {
        printf("\n");
        PrintLine();
        printf("  菜单\n");
        PrintLine();
        printf("  1. 读取所有英雄数据\n");
        printf("  2. 调试偏移链\n");
        printf("  3. 持续监控 (每2秒刷新)\n");
        printf("  0. 退出\n");
        PrintLine();
        printf("请选择: ");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                ReadAllHeroes();
                break;
            case 2:
                DebugOffsetChain();
                break;
            case 3:
                printf("\n开始持续监控 (Ctrl+C 退出)...\n");
                while (true) {
                    printf("\033[2J\033[H");  // 清屏
                    ReadAllHeroes();
                    sleep(2);
                }
                break;
            case 0:
                printf("\n再见！\n");
                delete driver;
                return 0;
            default:
                printf("无效选择\n");
        }
    }

    return 0;
}
