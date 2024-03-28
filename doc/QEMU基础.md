## QEMU介绍

> QEMU 是一个通用的开源机器模拟器和虚拟器。
>
> QEMU主要有两种比较常见的运作模式：User Mode（使用者模式）、System Mode（系统模式）。
>
> User Mode模式下，用户只需要将各种不同平台的处理编译得到的Linux程序放在QEMU虚拟中运行即可，其他的事情全部由QEMU虚拟机来完成，不需要用户自定义内核和虚拟磁盘等文件；
>
> System Mode模式下，最明显的特点是用户可以为QEMU虚拟机指定运行的内核或者虚拟硬盘等文件，简单来说系统模式下QEMU虚拟机是可根据用户的要求配置的。

## 一些系统模式下常用的QEMU选项：

```bash
-hda <disk-image>：指定一个磁盘镜像作为虚拟机的主硬盘。
-cdrom <cd-image>：指定一个CD-ROM镜像作为虚拟机的光驱。
-boot <order>：指定启动顺序，例如-boot c表示从主硬盘启动，-boot d表示从CD-ROM启动。
-m <memory>：设置虚拟机的内存大小。
-smp <cores>：指定虚拟机中的CPU数量。
-vga <driver>：指定图形显示驱动程序。
-serial <device>：指定串口设备，例如-serial stdio表示使用标准输入/输出流作为串口设备。
-net <option>：指定虚拟网络选项，例如-net nic表示模拟一个网络接口卡。
-monitor <device>：指定监视器设备，例如-monitor telnet:localhost:4444,server,nowait表示使用Telnet监视器。
-enable-kvm：启用KVM硬件加速，提高虚拟机性能。
-device <device>：指定虚拟设备，例如-device virtio-net-pci表示添加一个virtio网络接口卡。这是一个使用QEMU虚拟化软件来启动一个运行LoongArch64架构的虚拟机的命令行调用。
```

## qemu-system-loongarch64运行linux-loongarch64时的选项：

```shell
./build/qemu-system-loongarch64 -machine virt -m 4G -cpu la464 -smp 1 \
​    -bios ../qemu-binary/QEMU_EFI.fd \
​    -kernel ../linux/arch/loongarch/boot/vmlinuz.efi \
​    -initrd ../qemu-binary/ramdisk \
​    -serial stdio -monitor telnet:localhost:4495,server,nowait \
​    -append "root=/dev/ram rdinit=/sbin/init console=ttyS0,115200" \
​    -nographic
```

## 各个选项和参数的解释：

```bash
./build/qemu-system-loongarch64：指定了LoongArch64体系结构的QEMU二进制文件的路径。
-machine virt：选择要模拟的机器（或虚拟机）类型。
-m 4G：设置虚拟机的内存大小为4GB。
-cpu la464：指定要模拟的CPU型号为la464。
-smp 1：指定虚拟机中的CPU数量为1。
-bios ../qemu-binary/QEMU_EFI.fd： 指定用作虚拟机BIOS的文件路径。
-kernel ../linux/arch/loongarch/boot/vmlinuz.efi： 指定虚拟机将使用的内核文件路径。
-initrd ../qemu-binary/ramdisk： 指定用作虚拟机初始根文件系统的ramdisk（将RAM模拟为硬盘）文件路径。
-serial stdio： 启用标准输入/输出流的串行通信。
-monitor telnet:localhost:4495,server,nowait：启用一个Telnet监视器，用于远程控制虚拟机。
-append "root=/dev/ram rdinit=/sbin/init console=ttyS0,115200"：将一些内核启动参数添加到命令行中，例如root设备、初始进程、控制台等。
-nographic：禁用图形显示并使用纯文本控制台。
```
