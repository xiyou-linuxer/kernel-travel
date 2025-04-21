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

	config = read_cpucfg(LOONGARCH_CPUCFG1);
	if (config & CPUCFG2_PTW) {
		printk("have PTW configuration\n");
	}

	printk("@@@@@: fw_arg0 = %lx\n", fw_arg0);
	printk("@@@@@: fw_arg1 = %lx\n", fw_arg1);
	printk("@@@@@: fw_arg2 = %lx\n", fw_arg2);
	printk("@@@@@: fw_arg3 = %lx\n", fw_arg3);

#ifdef CONFIG_VIRT
#elif CONFIG_2K1000LA
	early_init();
#endif

	#endif

	/**
	 * 例外与中断的初始化
	 */
	per_cpu_trap_init(0);
	// init_environ();

	memblock_init();
	phy_pool_init();
	tlb_init(smp_processor_id());
	// paging_init();
}
