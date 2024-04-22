#include <linux/irqflags.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/task.h>
#include <linux/init.h>
#include <linux/ns16550a.h>
#include <linux/types.h>
#include <linux/stdio.h>
#include <linux/smp.h>
#include <asm-generic/bitsperlong.h>
#include <trap/irq.h>
#include <asm/pci.h>
#include <asm/setup.h>
#include <asm/bootinfo.h>
#include <asm/boot_param.h>
#include <linux/ahci.h>
#include <linux/block_device.h>
extern void __init __no_sanitize_address start_kernel(void);

bool early_boot_irqs_disabled;

extern void irq_init(void);
extern void setup_arch(void);
extern void trap_init(void);

void __init __no_sanitize_address start_kernel(void)
{
	char str[] = "xkernel";
	int cpu = smp_processor_id();
	unsigned long time;

	// serial_ns16550a_init(9600);
	printk("%s %s-%d.%d.%d\n", "hello", str, 0, 0, 1);
	// printk("@@@@@@: %d\n", BITS_PER_LONG);
	setup_arch();//初始化体系结构
	//初始化中断处理程序
	trap_init();
	irq_init();
	local_irq_enable();
	pci_init();
    disk_init();
	char buf[10000];
    block_read(0,1,buf,1);
    printk("buf:%s\n", buf);
    for (int i = 0; i < 1024; i++)
    {
        printk("%x ", buf[i]);
        if (i%8==0)
        {
            printk("\n");
        }
    }
        // local_irq_disable();

        // early_boot_irqs_disabled = true;

	/**
	 * 禁止中断，进行必要的设置后开启
	 */
	
	
	printk("cpu = %d\n", cpu);

	while (1) {
		time = csr_read64(LOONGARCH_CSR_TVAL);
		// printk("%lu\n",time);
	}
}
