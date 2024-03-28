# XOS-loongarch64
> 假设当前工作目录为 loongson

进入工作目录

```bash 
cd loongarch
```

### 1 环境工具准备

#### 1.1 配置支持 loongarch64 的 qemu

1. 下载 qemu，并进入

```bash
git clone  https://github.com/Qiubomm-OS/qemu.git
cd qemu
```

2. 配置

```bash
./configure
```

3. 编译 & 安装

```bash
make
sudo make install
```

#### 1.2 配置交叉工具链

回到工作目录 loongson 下。

##### 1.2.1 下载

点击下载交叉工具链：
[loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz](https://github.com/Qiubomm-OS/toolchains/releases/download/v0.1/loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz)

将交叉工具链包移动到工作目录 loongson 下。

##### 1.2.2 工具解压到工作目录下

```bash
tar -vxf loongarch64-clfs-6.3-cross-tools-gcc-full.tar.xz
```

> 可使用 -C 指定目标目录

### 2 获取UEFI启动引导

回到工作目录 loongson 下。

这里提供制作好的UEFI启动引导，下载地址：
[QEMU_EFI.fd](https://github.com/Qiubomm-OS/toolchains/releases/download/v0.1/QEMU_EFI.fd)

将UEFI启动引导移动到工作目录 loongson 下。

### 3 编译XOS-loongarch64

回到工作目录 loongson 下。

1. 下载

```bash
git clone https://github.com/Qiubomm-OS/XOS-loongarch64.git
```

2. 编译

```bash
cd XOS-loongarch64
bash quick_start.sh defconfig
bash quick_start.sh image
```

### 4 启动运行

进入 loongson/XOS-loongarch64 目录。

```bash
bash quick_start.sh run
```

### 5 清空生成的文件
```bash
bash quick_start.sh distclean
```
