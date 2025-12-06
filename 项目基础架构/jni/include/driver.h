/**
 * ============================================================================
 * 用户程序 - 内核驱动 交互示例
 * ============================================================================
 *
 * 【交互架构图】
 *
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │                        用户空间 (User Space)                            │
 *   │                                                                         │
 *   │   ┌─────────────────┐                                                   │
 *   │   │   用户程序       │  1. open("/dev/xxx") 打开驱动设备节点            │
 *   │   │   (main.cpp)    │  2. ioctl(fd, 命令码, &数据结构) 发送请求         │
 *   │   │                 │  3. 驱动返回读取的数据到buffer                    │
 *   │   └────────┬────────┘                                                   │
 *   │            │                                                            │
 *   │            │ ioctl() 系统调用                                           │
 *   └────────────┼────────────────────────────────────────────────────────────┘
 *                │
 *   ┌────────────┼────────────────────────────────────────────────────────────┐
 *   │            ▼                      内核空间 (Kernel Space)               │
 *   │   ┌─────────────────┐                                                   │
 *   │   │   内核驱动       │  4. 驱动接收ioctl请求                            │
 *   │   │   (xxx.ko)      │  5. 根据命令码执行对应操作                        │
 *   │   │                 │  6. copy_from_user 获取用户传入的参数             │
 *   │   └────────┬────────┘  7. 读取目标进程内存                              │
 *   │            │           8. copy_to_user 将数据返回给用户程序             │
 *   │            │                                                            │
 *   │            ▼                                                            │
 *   │   ┌─────────────────┐                                                   │
 *   │   │  目标进程内存    │  实际的游戏进程内存空间                           │
 *   │   │  (游戏进程)      │                                                   │
 *   │   └─────────────────┘                                                   │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *
 *
 * 【核心概念解释】
 *
 * 1. 设备节点 (/dev/xxx)
 *    - 驱动加载后会在/dev目录下创建一个字符设备文件
 *    - 用户程序通过open()打开这个文件获取文件描述符fd
 *    - 这个fd就是用户程序与内核驱动通信的"通道"
 *
 * 2. ioctl() 系统调用
 *    - ioctl(文件描述符, 命令码, 参数指针)
 *    - 命令码: 定义要执行什么操作 (如读取内存、写入内存、获取基址等)
 *    - 参数指针: 指向包含操作所需数据的结构体
 *
 * 3. 命令码 (IOCTL Code)
 *    - 0x800: 初始化/验证密钥
 *    - 0x801: 读取内存 (OP_READ_MEM)
 *    - 0x802: 写入内存 (OP_WRITE_MEM)
 *    - 0x803: 获取模块基地址 (OP_MODULE_BASE)
 *
 * 4. 数据结构
 *    - COPY_MEMORY: 用于读写内存操作
 *    - MODULE_BASE: 用于获取模块基地址
 *
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ctype.h>

/**
 * ============================================================================
 * 第一部分: 驱动通信的数据结构定义
 * ============================================================================
 *
 * 这些结构体是用户程序和内核驱动之间传递数据的"协议"
 * 用户程序填充结构体 -> ioctl传递给驱动 -> 驱动读取/填充数据 -> 返回给用户程序
 */

/**
 * COPY_MEMORY - 内存读写操作的数据结构
 *
 * 【使用流程】
 * 用户程序:
 *   1. 设置 pid = 目标进程ID
 *   2. 设置 addr = 要读取的内存地址
 *   3. 设置 buffer = 接收数据的缓冲区指针
 *   4. 设置 size = 要读取的字节数
 *   5. 调用 ioctl(fd, OP_READ_MEM, &cm)
 *
 * 内核驱动:
 *   1. copy_from_user 获取这个结构体
 *   2. 根据pid找到目标进程的mm_struct
 *   3. 使用内核函数读取指定地址的内存
 *   4. copy_to_user 将数据写入buffer
 */
typedef struct _COPY_MEMORY {
    pid_t pid;          // 目标进程的进程ID (Process ID)
    uintptr_t addr;     // 要读取/写入的内存地址
    void* buffer;       // 数据缓冲区指针
    size_t size;        // 要读取/写入的字节数
} COPY_MEMORY;

/**
 * MODULE_BASE - 获取模块基地址的数据结构
 *
 * 【为什么需要模块基地址?】
 * 游戏的so库(如libil2cpp.so)每次加载地址可能不同(ASLR)
 * 所以需要先获取基地址,然后 基地址+偏移 才能得到真实地址
 *
 * 【使用流程】
 * 用户程序:
 *   1. 设置 pid = 目标进程ID
 *   2. 设置 name = 模块名称 (如 "libil2cpp.so")
 *   3. 调用 ioctl(fd, OP_MODULE_BASE, &mb)
 *   4. 读取 mb.base 获取模块基地址
 */
typedef struct _MODULE_BASE {
    pid_t pid;          // 目标进程的进程ID
    char* name;         // 模块名称 (如 "libil2cpp.so")
    uintptr_t base;     // 返回的模块基地址
} MODULE_BASE;


/**
 * ============================================================================
 * 第二部分: ioctl 命令码定义
 * ============================================================================
 *
 * 命令码是用户程序告诉驱动"要执行什么操作"的标识
 * 不同的驱动可能使用不同的命令码,需要与驱动端保持一致
 */
enum DRIVER_OPERATIONS {
    OP_INIT_KEY    = 0x800,   // 初始化密钥 (某些驱动需要验证)
    OP_READ_MEM    = 0x801,   // 读取内存
    OP_WRITE_MEM   = 0x802,   // 写入内存
    OP_MODULE_BASE = 0x803,   // 获取模块基地址
};


/**
 * ============================================================================
 * 第三部分: 驱动操作封装类
 * ============================================================================
 *
 * 这个类封装了与内核驱动通信的所有操作
 * 对外提供简洁的接口: read<T>(addr), write<T>(addr, value), getModuleBase(name)
 */
class Driver {
private:
    int fd;         // 驱动设备的文件描述符
    pid_t pid;      // 当前操作的目标进程ID

    /**
     * 查找驱动设备节点
     *
     * 【驱动查找逻辑】
     * 1. 先查找预定义的驱动名称 (如 wanbai, CheckMe 等)
     * 2. 如果找不到,则根据文件属性特征来识别:
     *    - 是字符设备 (S_ISCHR)
     *    - 文件大小为0
     *    - 属主为root (uid=0, gid=0)
     *    - 文件名长度<=9
     *    - 创建时间和访问时间一致
     */
    char* findDriverPath() {
        const char* dev_path = "/dev";
        DIR* dir = opendir(dev_path);
        if (dir == NULL) {
            printf("[-] 无法打开/dev目录\n");
            return NULL;
        }

        // 预定义的驱动名称列表
        const char* known_drivers[] = {"wanbai", "CheckMe", "Ckanri", "lanran", "video188"};
        int known_count = sizeof(known_drivers) / sizeof(known_drivers[0]);

        struct dirent* entry;
        char* file_path = NULL;

        while ((entry = readdir(dir)) != NULL) {
            // 跳过 . 和 ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // 构建完整路径
            size_t path_len = strlen(dev_path) + strlen(entry->d_name) + 2;
            file_path = (char*)malloc(path_len);
            snprintf(file_path, path_len, "%s/%s", dev_path, entry->d_name);

            // 方法1: 检查是否是已知驱动名称
            for (int i = 0; i < known_count; i++) {
                if (strcmp(entry->d_name, known_drivers[i]) == 0) {
                    printf("[+] 找到已知驱动: %s\n", file_path);
                    closedir(dir);
                    return file_path;
                }
            }

            // 方法2: 根据文件属性特征识别
            struct stat file_info;
            if (stat(file_path, &file_info) < 0) {
                free(file_path);
                file_path = NULL;
                continue;
            }

            // 检查是否是字符设备
            if (!S_ISCHR(file_info.st_mode)) {
                free(file_path);
                file_path = NULL;
                continue;
            }

            // 跳过包含特殊字符的文件名
            if (strchr(entry->d_name, '_') || strchr(entry->d_name, '-') || strchr(entry->d_name, ':')) {
                free(file_path);
                file_path = NULL;
                continue;
            }

            // 跳过标准IO
            if (strcmp(entry->d_name, "stdin") == 0 ||
                strcmp(entry->d_name, "stdout") == 0 ||
                strcmp(entry->d_name, "stderr") == 0) {
                free(file_path);
                file_path = NULL;
                continue;
            }

            size_t name_len = strlen(entry->d_name);

            // 检查特征: 大小为0, 属主为root, 名字长度<=9
            if (file_info.st_size == 0 &&
                file_info.st_uid == 0 &&
                file_info.st_gid == 0 &&
                name_len <= 9 &&
                file_info.st_atime == file_info.st_ctime) {

                printf("[+] 根据特征找到驱动: %s\n", file_path);
                closedir(dir);
                return file_path;
            }

            free(file_path);
            file_path = NULL;
        }

        closedir(dir);
        printf("[-] 未找到驱动设备\n");
        return NULL;
    }

public:
    /**
     * 构造函数 - 初始化驱动连接
     *
     * 【执行流程】
     * 1. 调用findDriverPath()查找驱动设备节点
     * 2. 使用open()打开设备,获取文件描述符fd
     * 3. 后续所有操作都通过这个fd与驱动通信
     */
    Driver() : fd(-1), pid(0) {
        char* driver_path = findDriverPath();
        if (driver_path != NULL) {
            // 打开驱动设备 (这是与驱动建立通信的第一步!)
            fd = open(driver_path, O_RDWR);
            if (fd > 0) {
                printf("[+] 驱动连接成功, fd=%d\n", fd);
            } else {
                printf("[-] 驱动打开失败\n");
            }
            free(driver_path);
        }
    }

    /**
     * 析构函数 - 关闭驱动连接
     */
    ~Driver() {
        if (fd > 0) {
            close(fd);
        }
    }

    /**
     * 检查驱动是否有效
     */
    bool isValid() {
        return fd > 0;
    }

    /**
     * 初始化目标进程
     *
     * 【执行流程】
     * 1. 使用pidof命令获取进程ID
     * 2. 保存pid供后续读写操作使用
     *
     * @param process_name 进程名称 (如 "com.tencent.tmgp.sgame")
     * @return 进程ID,失败返回0
     */
    pid_t initialize(const char* process_name) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "pidof %s", process_name);

        FILE* fp = popen(cmd, "r");
        if (fp == NULL) {
            return 0;
        }

        fscanf(fp, "%d", &this->pid);
        pclose(fp);

        printf("[+] 进程 %s 的PID: %d\n", process_name, this->pid);
        return this->pid;
    }

    /**
     * 读取内存 (原始版本)
     *
     * 【核心交互流程 - 这是理解用户程序与驱动交互的关键!】
     *
     * 1. 用户程序填充COPY_MEMORY结构体
     * 2. 调用ioctl(fd, OP_READ_MEM, &cm)
     *    - fd: 之前open()返回的文件描述符
     *    - OP_READ_MEM: 命令码,告诉驱动"我要读内存"
     *    - &cm: 指向COPY_MEMORY结构体的指针
     *
     * 3. 系统调用进入内核空间
     *
     * 4. 内核驱动的ioctl处理函数被调用:
     *    - copy_from_user(&kernel_cm, user_cm, sizeof(COPY_MEMORY))
     *    - 根据pid找到目标进程的内存描述符
     *    - 使用access_process_vm()或类似函数读取内存
     *    - copy_to_user(user_buffer, kernel_buffer, size)
     *
     * 5. ioctl返回,用户程序的buffer中已经有了读取的数据
     *
     * @param addr 要读取的内存地址
     * @param buffer 接收数据的缓冲区
     * @param size 要读取的字节数
     * @return 成功返回true,失败返回false
     */
    bool read(uintptr_t addr, void* buffer, size_t size) {
        if (fd <= 0 || pid == 0) {
            return false;
        }

        // 地址掩码处理 (64位系统通常只用低48位)
        addr = addr & 0xFFFFFFFFFFFF;

        // 填充数据结构
        COPY_MEMORY cm;
        cm.pid = this->pid;
        cm.addr = addr;
        cm.buffer = buffer;
        cm.size = size;

        // 发送ioctl请求给驱动
        // 这一行代码就是用户程序与内核驱动交互的核心!
        if (ioctl(fd, OP_READ_MEM, &cm) != 0) {
            return false;
        }

        return true;
    }

    /**
     * 模板化读取 - 更方便的接口
     *
     * 使用示例:
     *   int value = driver->read<int>(address);
     *   float pos = driver->read<float>(position_addr);
     *   uint64_t ptr = driver->read<uint64_t>(pointer_addr);
     */
    template<typename T>
    T read(uintptr_t addr) {
        T result = {};
        if (this->read(addr, &result, sizeof(T))) {
            return result;
        }
        return {};
    }

    /**
     * 写入内存 (原始版本)
     *
     * 流程与read类似,只是命令码改为OP_WRITE_MEM
     */
    bool write(uintptr_t addr, void* buffer, size_t size) {
        if (fd <= 0 || pid == 0) {
            return false;
        }

        COPY_MEMORY cm;
        cm.pid = this->pid;
        cm.addr = addr;
        cm.buffer = buffer;
        cm.size = size;

        if (ioctl(fd, OP_WRITE_MEM, &cm) != 0) {
            return false;
        }

        return true;
    }

    /**
     * 模板化写入
     */
    template<typename T>
    bool write(uintptr_t addr, T value) {
        return this->write(addr, &value, sizeof(T));
    }

    /**
     * 获取模块基地址
     *
     * 【为什么需要这个功能?】
     * 现代系统使用ASLR(地址空间布局随机化),每次程序运行时
     * so库的加载地址都不同。所以我们需要:
     * 1. 先获取模块基地址
     * 2. 然后用 基地址+偏移 计算出真实地址
     *
     * @param name 模块名称 (如 "libil2cpp.so")
     * @return 模块基地址,失败返回0
     */
    uintptr_t getModuleBase(const char* name) {
        if (fd <= 0 || pid == 0) {
            return 0;
        }

        MODULE_BASE mb;
        char name_buf[256];
        strcpy(name_buf, name);

        mb.pid = this->pid;
        mb.name = name_buf;
        mb.base = 0;

        if (ioctl(fd, OP_MODULE_BASE, &mb) != 0) {
            return 0;
        }

        return mb.base;
    }

    /**
     * 获取当前进程ID
     */
    pid_t getPid() {
        return this->pid;
    }
};


/**
 * ============================================================================
 * 第四部分: 辅助函数 - 通过/proc文件系统获取bss段地址
 * ============================================================================
 *
 * 【什么是bss段?】
 * bss段是程序中存放未初始化全局变量和静态变量的内存区域
 * 游戏的很多关键数据(如玩家对象、游戏状态)都存放在bss段
 *
 * 【为什么要获取bss地址?】
 * 偏移地址通常是相对于bss段的,所以需要:
 * bss基址 + 偏移 = 实际内存地址
 *
 * 【实现原理】
 * 读取 /proc/[pid]/maps 文件,找到指定模块后面的[anon:.bss]段
 */
uintptr_t getBssAddress(pid_t pid, const char* module_name) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    FILE* fp = fopen(maps_path, "r");
    if (fp == NULL) {
        printf("[-] 无法打开 %s\n", maps_path);
        return 0;
    }

    char line[512];
    int found_module = 0;
    uintptr_t bss_addr = 0;

    while (fgets(line, sizeof(line), fp)) {
        // 先找到目标模块
        if (strstr(line, module_name) != NULL) {
            found_module = 1;
            continue;
        }

        // 找到模块后,下一个[anon:.bss]就是我们要的
        if (found_module) {
            if (strstr(line, "[anon:.bss]") != NULL) {
                sscanf(line, "%lx-%*lx", &bss_addr);
                break;
            } else {
                // 如果遇到其他模块,重置状态
                found_module = 0;
            }
        }
    }

    fclose(fp);

    if (bss_addr != 0) {
        printf("[+] %s 的bss地址: 0x%lx\n", module_name, bss_addr);
    }

    return bss_addr;
}


/**
 * ============================================================================
 * 第五部分: 向量结构体定义 (用于存储坐标)
 * ============================================================================
 */

// 二维向量
struct Vector2 {
    float X;
    float Y;

    Vector2() : X(0), Y(0) {}
    Vector2(float x, float y) : X(x), Y(y) {}
};

// 三维向量 (Unity使用XZY坐标系)
struct Vector3 {
    float X;
    float Z;  // Unity中Z是高度
    float Y;

    Vector3() : X(0), Z(0), Y(0) {}
    Vector3(float x, float z, float y) : X(x), Z(z), Y(y) {}
};

#endif // DRIVER_H
