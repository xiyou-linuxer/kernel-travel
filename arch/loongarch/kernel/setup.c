#include <asm/bootinfo.h>
#include <asm/setup.h>
#include <asm/loongarch.h>
#include <asm/page.h>
#include "linux/stdio.h"

unsigned long fw_arg0, fw_arg1, fw_arg2;
unsigned long kernelsp;

void setup_arch(void)
{
	unsigned int config;
	config = read_cpucfg(LOONGARCH_CPUCFG1);
	if (config & CPUCFG2_PTW) {
		printk("have PTW configuration\n");
	}
	/**
	 * 例外与中断的初始化
	 */
	per_cpu_trap_init(0);
	// init_environ();

	memblock_init();
	phy_pool_init();
	
}
