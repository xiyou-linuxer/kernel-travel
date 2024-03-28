## qemu-system-loongarch64 运行 linux-loongarch64

> 假设当前工作目录位 loongson

进入工作目录

```bash 
cd loongarch
```

### 1 环境工具准备

#### 1.1 配置支持 loongarch64 的 qemu

1. 下载 qemu，并进入

```bash
git clone  https://github.com/loongson/qemu.git
cd qemu
```

2. 配置

```bash
./configure --disable-rdma --disable-pvrdma --prefix=/usr \
	--target-list="loongarch64-softmmu" \
	--disable-libiscsi --disable-libnfs --disable-libpmem \
	--disable-glusterfs --enable-libusb --enable-usb-redir \
	--disable-opengl --disable-xen --enable-spice \
	--enable-debug --disable-capstone --disable-kvm \
	--enable-profiler
```

3. 编译

```bash
make
```

> qemu生成位置： loongson/qemu/build/qemu-system-loongarch64
>
> loongson为上述工作目录

#### 1.2 配置交叉工具链

回到工作目录 loongson 下。

1. 下载

```bash
wget https://github.com/loongson/build-tools/releases/download/2022.09.06/loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz
```

2. 将工具解压到 /opt 目录下

```bash
tar -vxf loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz  -C /opt
```

3. 将工具环境配置到当前终端（每次重启电脑后重新配置）

```bash
export PATH=/opt/cross-tools/bin:$PATH; \
export LD_LIBRARY_PATH=/opt/cross-tools/lib:$LD_LIBRARY_PATH; \
export LD_LIBRARY_PATH=/opt/cross-tools/loongarch64-unknown-linux-gnu/lib/:$LD_LIBRARY_PATH
```

> 工具环境可直接配置进终端配置文件中。如有需要，自行配置。

### 2 编译内核

回到工作目录 loongson 下。

1. 下载

```bash
git clone https://github.com/loongson/linux.git
```

2. 编译

```bash
cd linux
sudo apt install flex bison libssl-dev ncurses-dev   # 安装依赖
git checkout loongarch-next   # 切换分支
make ARCH=loongarch CROSS_COMPILE=loongarch64-unknown-linux-gnu- loongson3_defconfig   # 配置生成.config文件
make ARCH=loongarch CROSS_COMPILE=loongarch64-unknown-linux-gnu-   # 编译
```

生成内核二进制位置：

```bash
loongson/linux/arch/loongarch/boot/vmlinuz.efi   # loongson 为上述工作目录
```

### 3 获取UEFI启动引导、根文件系统（rootfs）

回到工作目录 loongson 下。

initrd 通过 busybox 生成， 这里提供制作好的范例，下载地址：

```bash
git clone https://github.com/yangxiaojuan-loongson/qemu-binary
```

### 4 启动运行

进入 loongson/qemu 目录。

```bash
./build/qemu-system-loongarch64 -machine virt -m 4G -cpu la464 -smp 1 \
	-bios ../qemu-binary/QEMU_EFI.fd \
	-kernel ../linux/arch/loongarch/boot/vmlinuz.efi \
	-initrd ../qemu-binary/ramdisk \
	-serial stdio -monitor telnet:localhost:4495,server,nowait \
	-append "root=/dev/ram rdinit=/sbin/init console=ttyS0,115200" \
	-nographic
```
