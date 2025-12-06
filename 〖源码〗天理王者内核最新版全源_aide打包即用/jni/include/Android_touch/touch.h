/*
    By 开源 公益分享
   搜集各个公益文件分享
    更多公益开源 文件 @BYYXnb
    TG @BYYXnb QQ群 427975011
    
源码捐赠链接
https://v.youxi186.com/shop/YIZ6ZA6X

源码捐赠地址2：
https://shop.pp13.cn/shop/byyxnb

公益网盘资源分享
https://www.123865.com/s/NxyZVv-K44U3?提取码：QWT6

QQ群过验证[有的文件需要]
https://www.123865.com/s/f4A7Vv-Pqc0A?提取码：9IVI

由于易支付漏洞 如果进不去
如您需拿卡 点击 联系我
赞助链接已重新更新 

〖如果你点开提示 站点停止运行或者进不去，那么解决办法有两个

1.复制链接去别的浏览器打开
2.清除你浏览器的数据即可
*/
#ifndef NATIVESURFACE_TOUCH_H
#define NATIVESURFACE_TOUCH_H
#include <android/input.h>
#include <android/keycodes.h>
#include <cstring>
#include <cstdio>
#include <malloc.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sys/system_properties.h>
#include <linux/input.h>
#include <linux/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/uinput.h>
#include <imgui.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include "draw.h"
struct Vector2 {
    Vector2(float x, float y) {
        this->x = x;
        this->y = y;
    }
    Vector2() {}
    float x;
    float y;
    bool operator == (const Vector2 &t) const {
        if ( this->x == t.x && this->y == t.y ) return true;
        return false;
    }
    bool operator != (const Vector2 &t) const {
        if ( this->x != t.x || this->y != t.y ) return true;
        return false;
    }
};
using namespace std;
#define BITS_PER_LONG (sizeof(long) * 8)
#define test_bit(array, bit)    ((array[bit / BITS_PER_LONG] >> bit % BITS_PER_LONG) & 1)
#define NBITS(x)             ((((x)-1)/BITS_PER_LONG)+1)
int isa_event_device(const struct dirent* dir);
std::string getTouchScreenDevice();
Vector2 rotatePointx(uint32_t orientation, float x, float y, int32_t displayWidth = 0,int32_t displayHeight = 0);
Vector2 getTouchScreenDimension(int fd);
void touch_config();
void Init_touch_config();
void Touch_Init(int px, int py);
void Touch_Down(int x, int y);
void Touch_Move(int x, int y);
void Touch_Up();
#endif
