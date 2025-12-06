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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <fstream>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <iostream>
#include <fstream>
#include <sys/system_properties.h>
#include <ctime>
#include <main.h>
#include <stdlib.h>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include "TouchHelperA.h"

int 分辨率;
int main(int argc, char **argv){
    system("mkdir -p  /data/天理配置");
    
    std::freopen("/dev/null", "w", stderr);

    printf("\033[1;39m");//白
    cout<<"0--------无后台启动\n1--------有后台启动\n";
    printf("请选择运行方式[输入序号]\n");
    int wht=1;
    cin>>wht;
    if(wht==0){	
    pid_t pid = fork();
    if (pid > 0) {
    exit(0);
    } else if (pid == 0) {
    
    } else {
    fprintf(stderr, "创建子进程失败，请联系作者解决问题\n");
    exit(1);
    }
    }
    
    
    
	
	FPS限制.AotuFPS_init();
	FPS限制.setAffinity();
    screen_config();
	
    std::string processName = "com.tencent.tmgp.sgame";
 
    printf("\033[31;1m");
    
	system("am start -n com.tencent.tmgp.sgame/.SGameActivity >/dev/null 2>&1");
    
    printf("\033[31;1m");   
    
    init_screen_x = screen_x + screen_y;
    init_screen_y = screen_y + screen_x;
	if(!init_egl(init_screen_x,init_screen_y)) {
        exit(0);
    }
    
    
    ImGui_init();  
	//Touch_Init(screen_x,screen_y);
	Touch::Init({(float)screen_x, (float)screen_y}, false);
    Touch::setOrientation(Orientation);
	Init_touch_config(); 
    //new std::thread(AimBotAuto,ImGui::GetForegroundDrawList());
	
    while (1)
    {     
        tick();
        FPS限制.SetFps(FPS);
		FPF显示 = FPS限制.AotuFPS();       
    }
    shutdown();
    return 0;
}
