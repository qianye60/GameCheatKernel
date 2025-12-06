export PATH=/data/data/com.termux/files/home/android-ndk-r17/:$PATH
echo "配置环境变量完成..."
#echo $PATH
ndk-build clean
echo "清理编译残留完成..."
ndk-build
echo "构建程序完成..."