#!/system/bin/sh

# 定义ANSI转义码
ESC_SEQ="\x1b["
RESET_SEQ="${ESC_SEQ}0m"
COLOR_SEQ="${ESC_SEQ}38;5;"

# 定义颜色代码
COLOR_RED="${COLOR_SEQ}9m"
COLOR_GREEN="${COLOR_SEQ}10m"
COLOR_YELLOW="${COLOR_SEQ}11m"
COLOR_BLUE="${COLOR_SEQ}12m"

BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'
echo -e "${COLOR_YELLOW}→ 下方出现 Invalid argument 再试一次${RESET_SEQ}"
echo -e "${COLOR_YELLOW}→ OPPO Realme 一加 需要过签名验证 + 升级到安卓13${RESET_SEQ}"
echo -e "${COLOR_YELLOW}→ 开机一段时间后可能会刷不进，自动重启后再刷一遍即可${RESET_SEQ}"

if [ "$(id -u)" != "0" ]; then
  echo -e "${BLUE}错误提示:${NC} 请使用root权限运行此脚本。" 1>&2
  exit 1
fi

ERROR_MSG=$(insmod ./4.19.191.ko 2>&1)
if [ $? -eq 0 ]; then
  echo -e "${COLOR_GREEN}操作成功:${NC} 内核模块已经成功加载。"
else
    #echo -e "${COLOR_GREEN}驱动刷入成功！${RESET_SEQ}"
    echo -e "${COLOR_RED}刷入失败，请重启手机后再试一次，确定不行再换其他脚本。${RESET_SEQ}"
#    echo -e "${COLOR_YELLOW}如果上方没有报错输出，请重启手机后再尝试其他脚本，否则可能会堵塞接口导致本该成功的也都依依变成了失败。${RESET_SEQ}"
    echo 20秒后自动重启设备 ...
    sleep 20
    reboot
  exit 1
fi