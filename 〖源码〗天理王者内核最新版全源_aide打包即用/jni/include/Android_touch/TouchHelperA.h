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
#pragma once

#include <linux/input.h>
#include <vector>
#include <functional>
#include "VectorStruct.h"

namespace Touch {
    struct touchObj {
        Vector2 pos{};
        int id = 0;
        bool isDown = false;
    };

    struct Device {
        int fd;
        float S2TX;
        float S2TY;
        input_absinfo absX, absY;
        touchObj Finger[10];

        Device() { memset((void *) this, 0, sizeof(*this)); }
    };

    bool Init(const Vector2 &s, bool p_readOnly);

    void Close();

    void Down(float x, float y);

    void Move(float x, float y);

    void Up();

    void Move(touchObj *touch, float x, float y);

    void Upload();

    void SetCallBack(const std::function<void(std::vector<Device> *)> &cb);

    Vector2 Touch2Screen(const Vector2 &coord);

    Vector2 GetScale();

    void setOrientation(int orientation);

    void setOtherTouch(bool p_otherTouch);
}
