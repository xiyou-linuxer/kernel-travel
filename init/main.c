#include <linux/irqflags.h>
#include <linux/printk.h>
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
#include <timer.h>
#include <thread.h>

#include <linux/ahci.h>
extern void __init __no_sanitize_address start_kernel(void);

bool early_boot_irqs_disabled;

extern void irq_init(void);
extern void setup_arch(void);
extern void trap_init(void);


void thread_a(void* unused)
{
    printk("enter thread_a\n");
    while(1) {
        printk("a ");
    }
}

void thread_b(void* unused)
{
    printk("enter thread_b\n");
    while(1) {
        printk("b ");
    }
}

int add(int a,int b)
{
	return a+b;
}

void __init __no_sanitize_address start_kernel(void)
{
	printk("%lx\n", lalist_mem_map.map_count);
	printk("%lx\n", lalist_mem_map.map->mem_type);
	printk("%lx\n", lalist_mem_map.map->mem_start);
	char str[] = "xkernel";
	int cpu = smp_processor_id();
	unsigned long time;
	int t = add(1,2);
	// serial_ns16550a_init(9600);
	printk("%s %s-%d.%d.%d\n", "hello", str, 0, 0, 1);
	// printk("@@@@@@: %d\n", BITS_PER_LONG);
	setup_arch();//初始化体系结构
	//初始化中断处理程序
	trap_init();
	irq_init();
	local_irq_enable();
	pci_init();
	// local_irq_disable();
	
	// early_boot_irqs_disabled = true;

	/**
	 * 禁止中断，进行必要的设置后开启
	 */
	
	
	printk("cpu = %d\n", cpu);

	while (1) {
		//time = csr_read64(LOONGARCH_CSR_TVAL);
		//printk("%lu\n",ticks);
        printk("m ");
	}
}
