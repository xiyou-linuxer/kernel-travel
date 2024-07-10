# Busybox 适配



### busybox 编译成fat32文件格式

```
make defconfig
CONFIG_TC=y ==> CONFIG_TC=n

nvim scripts/kconfig/lxdialog/check-lxdialog.sh
main ==> int main()

make menuconfig 设置静态编译
make -j 16
make install

```



把busybox加载进内存，化作文件系统的一部分。然后执行可执行文件。