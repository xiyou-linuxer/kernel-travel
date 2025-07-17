#include <asm/bootinfo.h>
#include <asm/setup.h>
#include <asm/loongarch.h>
#include <asm/page.h>
#include <asm/tlb.h>
#include <xkernel/smp.h>
#include <xkernel/stdio.h>
#include <xkernel/init.h>

unsigned long fw_arg0, fw_arg1, fw_arg2, fw_arg3;
unsigned long kernelsp;

char __initdata arcs_cmdline[512];

void setup_arch(void)
{
	unsigned int config;
	// 读取龙芯 CPU 的配置寄存器 CPUCFG1。
	config = read_cpucfg(LOONGARCH_CPUCFG1);
	// 检查 CPU 是否支持页表遍历硬件 (Page Table Walker - PTW)。
	if (config & CPUCFG2_PTW) {
		printk("have PTW configuration\n");
	}

	printk("@@@@@: fw_arg0 = %lx\n", fw_arg0);
	printk("@@@@@: fw_arg1 = %lx\n", fw_arg1);
	printk("@@@@@: fw_arg2 = %lx\n", fw_arg2);
	printk("@@@@@: fw_arg3 = %lx\n", fw_arg3);

	// 执行更进一步的早期初始化。
	early_init();

	/**
	 * 例外与中断的初始化   初始化异常向量表 (trap vector)。
	 */
	per_cpu_trap_init(0);
	// init_environ();

	// memblock 是内核在早期启动阶段使用的内存管理器。
	memblock_init();
	// 初始化物理内存池。
	phy_pool_init();
	// CPU 的 TLB (Translation Lookaside Buffer)。
	tlb_init(smp_processor_id());
	// paging_init();
}
