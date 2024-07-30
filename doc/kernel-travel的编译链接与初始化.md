# kernel-travel 的编译链接与初始化

## 适应多架构的编译

对于 kernel-travel 内核镜像，我们用 makefile 进行递归式的构建。在顶级目录的 makefile 中指定了默认的编译架构与工具链路径。若在编译阶段没有指定 ARCH 等编译参数，则会按照默认的 loongarch 进行构建。

```makefile
export KBUILD_BUILDHOST := $(SUBARCH)
ARCH            ?= loongarch
CROSS_COMPILE   ?= /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-
UTS_MACHINE     := $(ARCH)
SRCARCH         := $(ARCH)
```

为了方便后期对于多架构的支持，我们规定与体系结构相关的代码都放在 arch 目录下。

并在 makefile 中提供如下规则：只有被 ARCH 指定架构目录下的代码才会被编译。

```makefile
# 添加体系结构特定的Makefile，SRCARCH 由 ARCH 指定
include $(srctree)/arch/$(SRCARCH)/Makefile
```

kernel-travel 通过以上构建规则，完成对多架构的适配。

## 链接

[内核链接脚本](../arch/loongarch/kernel/vmlinux.lds.S)

其中定义了如下内容：

* PECOFF 对齐
* 程序头定义
* 内存布局

最终在内核空间中形成如下内存布局：

```sh
+------------------------------------+
|          .text (代码段)            |  <- 起始地址: VMLINUX_LOAD_ADDRESS
|------------------------------------|
|      .text (代码段)                |  <- 可读、可写、可执行 (RWX)
|------------------------------------|
|      .fixup (修复)                 |
|      .gnu.warning (警告)           |
|      .altinstructions (备用指令)   |
|------------------------------------|
|      .got (全局偏移表)             |  <- 对齐到16字节
|------------------------------------|
|      .plt (过程链接表)             |
|------------------------------------|
|      .got.plt (PLT的GOT)           |
|------------------------------------|
|      .init.text (初始化代码)       |
|------------------------------------|
|      .exit.text (退出代码)         |
|------------------------------------|
|      .init.data (初始化数据)       |
|------------------------------------|
|      .exit.data (退出数据)         |
|------------------------------------|
|      .sdata (静态数据)             |  <- 可读/可写
|------------------------------------|
|      .edata (数据结束)             |
|------------------------------------|
|      .bss (未初始化数据)           |  <- 以0填充的内存区域
|------------------------------------|
|     .gptab.data (GPTAB数据)        |
|     .gptab.sdata (GPTAB静态数据)   |
|     .gptab.bss (GPTAB BSS)         |
|     .gptab.sbss (GPTAB静态BSS)     |
|------------------------------------|
|     .gnu.attributes (丢弃)         |
|     .options (丢弃)                |
|     .eh_frame (丢弃)               |
+------------------------------------+
```

## 启动流程

在 uboot 加载为内核映像之后，第一个被执行的函数为 `kernel_entry` 。

该函数作为内核的入口点函数，完成了以下四项工作：

* 配置直接映射窗口
* 设置CRMD寄存器以启用分页（PG）
* 清空.bss段
* 跳转到 `start_kernel` 执行

进入到 start_kernel 后对 xkernel 系统环境进行初始化。

```c
void __init __no_sanitize_address start_kernel(void)
{
	char str[] = "xkernel";
	int cpu = smp_processor_id();
	
	local_irq_disable();

	printk("%s %s-%d.%d.%d\n", "hello", str, 0, 0, 1);
	setup_arch();//初始化体系结构
	mem_init();
	trap_init();
	irq_init();

	thread_init();
	timer_init();
	pci_init();
	console_init();
	disk_init();
	console_init();
	syscall_init();
	char buf[512];
	vfs_init();
	fs_init();
	early_boot_irqs_disabled = true;
	int fd = sys_open("initcode",O_CREATE|O_RDWR,0);
	if (fd == -1) {
		printk("open failed");
	}
	sys_write(fd,init_code,init_code_len);
	bufSync();
	local_irq_enable();
	
	while (1) {

	}
}
```

### 对内存管理的初始化

对于内存管理的初始化主要包括以下三部分：

- 从设备树中获取 2k1000 的内存信息
  
  ```c
  void __init memblock_init(void);
  ```

- 初始化物理内存池
  
  ```c
  void __init phy_pool_init(void)
  ```

- 初始化 NUMA 架构下的 node 结构

  ```c
  void paging_init(void)
  ```

### 对异常处理的初始化

异常处理的初始化中包括了对于中断出处理、系统调用、tlb重填例外的初始化

```c
void tlb_init(int cpu);
void trap_init(void)
void irq_init();
void syscall_init();
```

### 线程运行环境的初始化与 init 进程

1. 初始化线程运行环境

   在函数 `thread_init` 中，对于就绪线程列表 `thread_ready_list` 和全部线程列表 `thread_all_list` 、pid 池，进行初始化。并启动了 `idle` 线程。

   ```c
   void thread_init();
   ```

2. init 进程

   init进程的代码位于 command 目录之下。这段代码在编译后被写入磁盘中，以便以后的使用。

   ```c
   int fd = sys_open("initcode",O_CREATE|O_RDWR,0);
    if (fd == -1) {
        printk("open failed");
    }
    sys_write(fd,init_code,init_code_len);
    bufSync();
   ```

### 对设备的初始化

在对于设备的初始化中，我们完成了对系统时钟、 pci 总线设备、磁盘控制器 SATA、通信串口 uart 的初始化。

```c
void timer_init();
void pci_init();
void disk_init(void);
void real_serial_ns16550a_init(uint64_t base_addr, uint32_t baud_rate);
```

### 对文件系统的初始化

对文件系统的初始化我们包括两个部分

1. 对于 vfs 层的初始化

   在 vfs_init 中完成了对于 dcache 的初始化，并创建了 vfs 层的根目录。

   ```c
   void vfs_init(void);
   ```

2. 对于磁盘文件的初始化

   在全国决赛中，我们默认的磁盘文件系统格式为 ext4 ，对于磁盘文件系统的初始化也相当于对 ext4 文件系统的初始化。

   ```c
   void fs_init(void);
   ```
