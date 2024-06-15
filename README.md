# kernel-travel

kernel-travel 是由 重启之我是loader 三名成员共同开发的基于 LoongArch 的架构的64位操作系统。系统采用 kbuild 编译框架，从 0 开发内核，部分头文件移植自 Linux 。

## 编译并运行该项目

### 1 环境工具准备

#### 拉取 Docker 镜像并启动

```bash
docker pull docker.educg.net/cg/os-contest:2024p6
docker run --privileged  -it --name os-contest -p 12306:22 docker.educg.net/cg/os-contest:2024p6 /bin/bash
```

#### 为 Docker 中的 qemu 配置内部网络

需要在 Docker 中安装的工具：

```sh
apt-get install bridge-utils        # 虚拟网桥工具
apt-get install uml-utilities       # UML（User-mode linux）工具
apt-get install net-tools 
```

配置命令：

```bash
ip link add br0 type bridge         #创建一个网络桥接接口
ip tuntap add dev tap0 mode tap     #创建一个 TAP 设备
brctl addif br0 tap0                #将 tap0 设备加入到 br0 网桥
ifconfig br0 0.0.0.0 promisc up     #将 br0 接口的 IP 地址设置为 0.0.0.0
ifconfig tap0 0.0.0.0 promisc up    #将 tap0 接口的 IP 地址设置为 0.0.0.0
ifconfig br0 10.0.0.1 netmask 255.255.255.0 #配置子网掩码
```

#### 启动 Docker 中的 qemu

生成2kfs.img：

在启动qemu之前要先运行脚本create_qemu_img.sh生成2kfs.img文件。

```bash
cd /tmp/qemu/2k1000
./create_qemu_img.sh
```

传入[sdcard.img](https://github.com/oscomp/testsuits-for-oskernel/blob/pre-2023/sdcard.img.gz)：

```sh
sudo docker cp 你存放sdcard-loongarch.img的路径 os-contest:/sdcard-loongarch.img
```

启动qemu

```bash
cd /tmp/qemu
./runqemu
```

#### 在 Docker 中安装 tftp 服务器并启动服务

在加载内核镜像时需要用到 tftp 服务

```bash
apt install tftpd-hpa
service tftpd-hpa start
```

### 2 获取UEFI启动引导

这里提供制作好的UEFI启动引导，下载地址：
[QEMU_EFI.fd](https://github.com/Qiubomm-OS/toolchains/releases/download/v0.1/QEMU_EFI.fd)

将UEFI启动引导移动到当前工作目录下。

### 3 编译kernel-travel

1. 下载

```bash
git clone https://github.com/xiyou-linuxer/kernel-travel.git
```

2. 配置交叉工具链

点击下载交叉工具链：
[loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz](https://github.com/Qiubomm-OS/toolchains/releases/download/v0.1/loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz)

将交叉工具链包解压到 kernel-travel 目录下（交叉编译工具较大，预留足够存储空间）。

```bash
tar -vxf loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz
```

3. 编译并将内核镜像传入 Docker 容器

```bash
cd kernel-travel 
bash quick_start.sh defconfig
bash quick_start.sh image
```

提醒：QEMU_EFI.fd路径和交叉工具链包路径可以自定义，修改`quick_start.sh`中`run()`以及TOOLCHAINS路径即可。

### 4 通过 tftp 服务启动内核

进入执行 ./runqemu 后进入 uboot 界面，执行如下命令：

```bash
setenv ipaddr 10.0.0.2                      #设置本机的 IP 地址为 10.0.0.2
setenv serverip 10.0.0.1                    #设置 TFTP 服务器的 IP 地址为 10.0.0.1
tftpboot 0x9000000008000000 10.0.0.1:Image  #从 TFTP 服务器下载名为 Image 的文件，并将其加载到指定的内存地址处
go 0x9000000008000000                       #跳转到指定地址执行
```

### 5 清空生成的文件

```bash
bash quick_start.sh distclean
```

## 文档列表

文档位于 kernel-travel/doc 目录下。

* [gdb调试方法](./doc/安装x86环境下支持调试loongarch体系结构的gdb.md)
* [任务的创建与切换](./doc/任务创建与切换.md)
* [异常处理与时间](./doc/异常处理与时间.md)
* [pci总线驱动](./doc/pci总线驱动.md)
* [磁盘驱动](./doc/磁盘驱动.md)
* [文件系统](./doc/文件系统.md)

## 参考资料

* [操作系统真相还原](https://github.com/yifengyou/os-elephant)
* [兰州大学 - MaQueOs](https://gitee.com/dslab-lzu/maqueos)
* [种田队 - 2023年优秀作品](https://gitlab.eduxiji.net/202310006101080/zhongtianos)
* [dis-sum](https://gitee.com/xiebaoyou/dim-sum)
* [Linux内核源码](https://elixir.bootlin.com/linux/latest/source)
