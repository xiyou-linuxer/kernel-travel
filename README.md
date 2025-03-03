# kernel-travel

## 项目简介

kernel-travel 是基于 LoongArch 的架构的64位操作系统。考虑到未来对于多架构的适配，该系统采用 kbuild 编译框架对项目进行构建，目前支持 LoongArch 架构的硬件开发板（2k1000、3A5000）。

kernel-travel 包含有五大模块，分别为：内存管理、进程管理、文件系统、设备驱动、系统调用。

![Alt text](./doc/img/系统架构.png)



## 文档列表

文档位于 kernel-travel/doc 目录下。

环境配置：

* [编译并运行kernel-travel](./doc/编译并运行kernel-travel.md)
* [gdb调试方法](./doc/安装x86环境下支持调试loongarch体系结构的gdb.md)
* [快速启动](./doc/快速启动.md)

进程管理：

* [任务的创建与切换](./doc/任务创建与切换.md)
* [异常处理与时间](./doc/异常处理与时间.md)

设备驱动：

* [pci总线驱动](./doc/pci总线驱动.md)
* [磁盘驱动](./doc/磁盘驱动.md)

内存管理：

* [虚拟内存管理](./doc/虚拟内存管理.md)
* [物理内存管理](./doc/物理内存管理.md)

文件系统：

* [Fat32文件系统.md](./doc/Fat32文件系统.md)
* [文件系统架构与VFS](./doc/文件系统.md)
* [适配lwext4.md](./doc/适配lwext4.md)

其他：

* [kernel-travel的编译链接与初始化](./doc/kernel-travel的编译链接与初始化.md)
* [适配busybox](./doc/适配busybox.md)
* [适配硬件开发板](./doc/板子.md)

## 参考资料

* [操作系统真相还原](https://github.com/yifengyou/os-elephant)
* [兰州大学 - MaQueOs](https://gitee.com/dslab-lzu/maqueos)
* [种田队 - 2023年优秀作品](https://gitlab.eduxiji.net/202310006101080/zhongtianos)
* [dim-sum](https://gitee.com/xiebaoyou/dim-sum)
* [Linux内核源码](https://elixir.bootlin.com/linux/latest/source)
