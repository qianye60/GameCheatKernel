/**
 * 偏移验证工具 - 验证新发现的偏移
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "driver.h"

Driver* driver = nullptr;
pid_t gamePid = 0;
uintptr_t libGameCore_base = 0;
uintptr_t libGameCore_bss = 0;

// 新发现的偏移
namespace NewOffset {
    const uintptr_t GameManager = 0x2728;    // BSS + 0x2728
    const uintptr_t HeroListPtr = 0x8;       // +0x8 -> 英雄列表
    const uintptr_t HeroInterval = 0x18;     // 每个英雄间隔
    const uintptr_t HeroObject = 0x68;       // +0x68 -> 英雄对象
    const uintptr_t HeroId = 0x30;           // 英雄ID
    const uintptr_t HeroCamp = 0x3C;         // 阵营
}

uintptr_t ReadPtr(uintptr_t addr) {
    if (addr == 0) return 0;
    addr = addr & 0xFFFFFFFFFFFF;
    if (addr < 0x1000000000 || addr > 0xFFFFFFFFFFFF) return 0;
    uint64_t val = driver->read<uint64_t>(addr);
    return val & 0xFFFFFFFFFFFF;
}

int ReadInt(uintptr_t addr) {
    if (addr == 0) return 0;
    return driver->read<int>(addr);
}

bool Initialize() {
    printf("========================================\n");
    printf("  偏移验证工具\n");
    printf("========================================\n\n");

    driver = new Driver();
    if (!driver->isValid()) {
        printf("[-] 驱动连接失败\n");
        return false;
    }

    gamePid = driver->initialize("com.tencent.tmgp.sgame");
    if (gamePid == 0) {
        printf("[-] 未找到游戏进程\n");
        return false;
    }

    libGameCore_base = driver->getModuleBase("libGameCore.so");
    libGameCore_bss = getBssAddress(gamePid, "libGameCore.so");

    printf("[+] PID: %d\n", gamePid);
    printf("[+] base: 0x%lx\n", libGameCore_base);
    printf("[+] bss:  0x%lx\n\n", libGameCore_bss);

    return true;
}

/**
 * 使用新偏移读取所有英雄
 */
void ReadAllHeroes() {
    printf("========================================\n");
    printf("使用新偏移读取英雄数据\n");
    printf("========================================\n");
    printf("偏移链: BSS+0x%lx -> +0x%lx -> [i*0x%lx] -> +0x%lx\n\n",
           NewOffset::GameManager, NewOffset::HeroListPtr,
           NewOffset::HeroInterval, NewOffset::HeroObject);

    // 第1步: BSS + 0x2728
    uintptr_t gameManager = ReadPtr(libGameCore_bss + NewOffset::GameManager);
    printf("[1] BSS + 0x2728 = 0x%lx\n", gameManager);
    if (gameManager == 0) {
        printf("[-] 游戏管理器为空\n");
        return;
    }

    // 第2步: +0x8 获取英雄列表
    uintptr_t heroList = ReadPtr(gameManager + NewOffset::HeroListPtr);
    printf("[2] +0x8 = 0x%lx (英雄列表)\n\n", heroList);
    if (heroList == 0) {
        printf("[-] 英雄列表为空\n");
        return;
    }

    printf("%-4s %-8s %-6s %-20s\n", "序号", "英雄ID", "阵营", "英雄对象地址");
    printf("----------------------------------------\n");

    // 遍历英雄数组
    int validCount = 0;
    for (int i = 0; i < 10; i++) {
        // [i * 0x18]
        uintptr_t heroArrayEntry = ReadPtr(heroList + i * NewOffset::HeroInterval);
        if (heroArrayEntry == 0) continue;

        // +0x68
        uintptr_t heroPtr = ReadPtr(heroArrayEntry + NewOffset::HeroObject);
        if (heroPtr == 0) continue;

        // 读取英雄ID和阵营
        int heroId = ReadInt(heroPtr + NewOffset::HeroId);
        int camp = ReadInt(heroPtr + NewOffset::HeroCamp);

        if (heroId > 0 && heroId < 1000 && (camp == 1 || camp == 2)) {
            const char* campStr = (camp == 1) ? "蓝方" : "红方";
            printf("%-4d %-8d %-6s 0x%lx\n", i, heroId, campStr, heroPtr);
            validCount++;
        }
    }

    printf("----------------------------------------\n");
    printf("共找到 %d 个有效英雄\n\n", validCount);
}

/**
 * 尝试读取坐标 - 测试不同的坐标偏移
 */
void TryReadCoordinates() {
    printf("========================================\n");
    printf("尝试读取坐标\n");
    printf("========================================\n");

    uintptr_t gameManager = ReadPtr(libGameCore_bss + NewOffset::GameManager);
    if (gameManager == 0) return;

    uintptr_t heroList = ReadPtr(gameManager + NewOffset::HeroListPtr);
    if (heroList == 0) return;

    // 读取第一个有效英雄
    for (int i = 0; i < 10; i++) {
        uintptr_t heroArrayEntry = ReadPtr(heroList + i * NewOffset::HeroInterval);
        if (heroArrayEntry == 0) continue;

        uintptr_t heroPtr = ReadPtr(heroArrayEntry + NewOffset::HeroObject);
        if (heroPtr == 0) continue;

        int heroId = ReadInt(heroPtr + NewOffset::HeroId);
        int camp = ReadInt(heroPtr + NewOffset::HeroCamp);

        if (heroId > 0 && heroId < 1000 && (camp == 1 || camp == 2)) {
            printf("英雄ID=%d, 阵营=%d\n", heroId, camp);
            printf("英雄对象地址: 0x%lx\n\n", heroPtr);

            // 尝试不同的坐标偏移
            printf("尝试坐标偏移 (旧版本是 +0x248 -> +0x10 -> +0x0 -> +0x60):\n\n");

            // 尝试常见的坐标指针偏移
            int coordOffsets[] = {0x240, 0x248, 0x250, 0x258, 0x260, 0x268, 0x270, 0x278, 0x280};

            for (int j = 0; j < sizeof(coordOffsets)/sizeof(coordOffsets[0]); j++) {
                int off = coordOffsets[j];
                uintptr_t ptr1 = ReadPtr(heroPtr + off);
                if (ptr1 == 0) continue;

                uintptr_t ptr2 = ReadPtr(ptr1 + 0x10);
                if (ptr2 == 0) continue;

                uintptr_t ptr3 = ReadPtr(ptr2);
                if (ptr3 == 0) continue;

                uintptr_t coordPtr = ReadPtr(ptr3 + 0x60);
                if (coordPtr == 0) continue;

                int x = ReadInt(coordPtr);
                int y = ReadInt(coordPtr + 0x8);

                // 坐标范围检查 (王者地图大约 0-30000)
                if (x > 0 && x < 50000 && y > 0 && y < 50000) {
                    printf("[可能] +0x%x -> +0x10 -> +0x0 -> +0x60\n", off);
                    printf("        坐标: X=%d, Y=%d\n\n", x, y);
                }
            }

            // 也尝试直接在英雄对象上找坐标
            printf("尝试直接偏移:\n");
            for (int off = 0x100; off < 0x400; off += 8) {
                int x = ReadInt(heroPtr + off);
                int y = ReadInt(heroPtr + off + 4);

                // 坐标范围检查
                if (x > 1000 && x < 30000 && y > 1000 && y < 30000) {
                    printf("  +0x%x: X=%d, Y=%d\n", off, x, y);
                }
            }

            break;  // 只分析第一个英雄
        }
    }
}

/**
 * 尝试读取血量
 */
void TryReadHp() {
    printf("\n========================================\n");
    printf("尝试读取血量\n");
    printf("========================================\n");

    uintptr_t gameManager = ReadPtr(libGameCore_bss + NewOffset::GameManager);
    if (gameManager == 0) return;

    uintptr_t heroList = ReadPtr(gameManager + NewOffset::HeroListPtr);
    if (heroList == 0) return;

    for (int i = 0; i < 10; i++) {
        uintptr_t heroArrayEntry = ReadPtr(heroList + i * NewOffset::HeroInterval);
        if (heroArrayEntry == 0) continue;

        uintptr_t heroPtr = ReadPtr(heroArrayEntry + NewOffset::HeroObject);
        if (heroPtr == 0) continue;

        int heroId = ReadInt(heroPtr + NewOffset::HeroId);
        int camp = ReadInt(heroPtr + NewOffset::HeroCamp);

        if (heroId > 0 && heroId < 1000 && (camp == 1 || camp == 2)) {
            printf("\n英雄ID=%d, 阵营=%d\n", heroId, camp);

            // 尝试血量偏移 (旧版本是 +0x168 -> +0xa8/+0xb0)
            int hpStructOffsets[] = {0x160, 0x168, 0x170, 0x178, 0x180};

            for (int j = 0; j < sizeof(hpStructOffsets)/sizeof(hpStructOffsets[0]); j++) {
                int off = hpStructOffsets[j];
                uintptr_t hpStruct = ReadPtr(heroPtr + off);
                if (hpStruct == 0) continue;

                // 尝试不同的血量偏移
                int hpOffsets[] = {0xa0, 0xa8, 0xb0, 0xb8, 0xc0};
                for (int k = 0; k < sizeof(hpOffsets)/sizeof(hpOffsets[0]); k++) {
                    int hp1 = ReadInt(hpStruct + hpOffsets[k]);
                    int hp2 = ReadInt(hpStruct + hpOffsets[k] + 8);

                    // 血量范围检查 (一般 100-20000)
                    if (hp1 > 100 && hp1 < 30000 && hp2 > 100 && hp2 < 30000) {
                        printf("  [可能] +0x%x -> +0x%x/+0x%x: %d/%d\n",
                               off, hpOffsets[k], hpOffsets[k]+8, hp1, hp2);
                    }
                }
            }
        }
    }
}

/**
 * 打印英雄对象的完整数据
 */
void DumpHeroData() {
    printf("\n========================================\n");
    printf("导出英雄对象数据\n");
    printf("========================================\n");

    uintptr_t gameManager = ReadPtr(libGameCore_bss + NewOffset::GameManager);
    if (gameManager == 0) return;

    uintptr_t heroList = ReadPtr(gameManager + NewOffset::HeroListPtr);
    if (heroList == 0) return;

    // 找第一个有效英雄
    for (int i = 0; i < 10; i++) {
        uintptr_t heroArrayEntry = ReadPtr(heroList + i * NewOffset::HeroInterval);
        if (heroArrayEntry == 0) continue;

        uintptr_t heroPtr = ReadPtr(heroArrayEntry + NewOffset::HeroObject);
        if (heroPtr == 0) continue;

        int heroId = ReadInt(heroPtr + NewOffset::HeroId);
        int camp = ReadInt(heroPtr + NewOffset::HeroCamp);

        if (heroId > 0 && heroId < 1000 && camp == 1) {  // 只看蓝方英雄
            printf("英雄ID=%d (后羿=169), 阵营=%d\n", heroId, camp);
            printf("英雄对象: 0x%lx\n\n", heroPtr);

            // 打印前 0x300 字节的指针数据
            printf("指针数据 (可能的结构体指针):\n");
            for (int off = 0; off < 0x300; off += 8) {
                uintptr_t ptr = ReadPtr(heroPtr + off);
                if (ptr != 0 && ptr > 0x1000000000) {
                    printf("  +0x%03x: 0x%lx\n", off, ptr);
                }
            }

            break;
        }
    }
}

int main() {
    if (!Initialize()) {
        return -1;
    }

    // 验证新偏移
    ReadAllHeroes();

    // 尝试读取坐标
    TryReadCoordinates();

    // 尝试读取血量
    TryReadHp();

    // 导出英雄数据
    DumpHeroData();

    printf("\n========================================\n");
    printf("验证完成\n");
    printf("========================================\n");

    delete driver;
    return 0;
}
