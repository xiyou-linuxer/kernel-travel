# 编译并运行该项目

## 1 环境工具准备

### 拉取 Docker 镜像并启动

```bash
docker pull docker.educg.net/cg/os-contest:2024p6				#拉取镜像
sudo docker run --privileged  -it -v ~/os-contest2025:/xkernel --name os-contest -p 12306:22 docker.educg.net/cg/os-contest:2024p6 /bin/bash		#运行容器
```

### 为 Docker 中的 qemu 配置内部网络

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

需要在宿主机中安装的工具：

```bash
sudo pacman -S qemu-img qemu-common nbd e2fsprogs cpio xz
```

## 2 编译kernel-travel

编译kernel-travel是在docker中进行的：

* 克隆代码

```bash
#放置在os-contest目录中
git clone https://github.com/xiyou-linuxer/kernel-travel.git
```

* 下载工具

将`kernel-travel`中`arch/loongarch/boot`中的makefile中路径进行替换

```makefile
entry-y	= $(shell /xkernel/linux/linux-5.10-2k1000-dp-src/arch/loongarch/tools/elf-entry vmlinux)
# 替换为本地路径(在docker中的路径)
```

* 脚本运行

```bash
cd /xkernel/kernel-travel
./compile.sh
```

* 编译内核

```bash
cd /xkernel/kernel-travel5
ARCH=loongarch CROSS_COMPILE=/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu- make defconfig
ARCH=loongarch CROSS_COMPILE=/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu- make uImage
```

## 3 启动内核

生成2kfs.img

以下命令是在docker容器内完成的：

```bash
cd /tmp/qemu/2k1000
cp rootfs-la.cpio.lzma /xkernel
cp uImage /xkernel
```

以下命令是在宿主机中完成的：

```bash
cd ~/os-contest2025

sudo modprobe nbd max_part=12

qemu-img create -f qcow2 2kfs.img 2G

sudo qemu-nbd -c /dev/nbd0 ./2kfs.img

echo -e 'n\n\n\n\n\n\nw\nq\n' | sudo fdisk /dev/nbd0
sudo partprobe /dev/nbd0

sudo mkfs.ext4 /dev/nbd0p1

sudo mkdir -p ./mnt
sudo mount /dev/nbd0p1 ./mnt

bash -c "lzcat ./rootfs-la.cpio.lzma | cpio -idmv -D /mnt/2kfs &> ./cpio.log"

sudo mkdir -p ./mnt/boot
sudo cp kernel-travel/arch/loongarch/boot/uImage ./mnt/boot/

sudo docker cp 2kfs.img os-contest:/tmp/qemu/2k1000/
```

### 运行qemu

在docker容器中：

```bash
cd /xkernel/kernel-travel
./runqemu.sh
```



