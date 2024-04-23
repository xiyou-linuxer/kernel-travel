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
#include <asm/timer.h>
#include <linux/thread.h>
#include <linux/ahci.h>
#include <sync.h>
extern void __init __no_sanitize_address start_kernel(void);

bool early_boot_irqs_disabled;

extern void irq_init(void);
extern void setup_arch(void);
extern void trap_init(void);


void thread_a(void* unused)
{
    printk("enter thread_a\n");
    while (1) {
        unsigned long crmd = read_csr_crmd();
        printk("thread_a at:at pri %d  ",crmd&PLV_MASK);
    }
}

//void proc_1(void* unused)
//{
//    printk("enter proc_1\n");
//    while(1) {
//        unsigned long crmd = read_csr_crmd();
//        printk("oo");
//        printk("proc_1:at pri %d  ",crmd&PLV_MASK);
//    }
//}

void __init __no_sanitize_address start_kernel(void)
{
	printk("%lx\n", lalist_mem_map.map_count);
	printk("%lx\n", lalist_mem_map.map->mem_type);
	printk("%lx\n", lalist_mem_map.map->mem_start);
	char str[] = "xkernel";
	int cpu = smp_processor_id();
	unsigned long time;
	// serial_ns16550a_init(9600);
	printk("%s %s-%d.%d.%d\n", "hello", str, 0, 0, 1);
	setup_arch();//初始化体系结构
	//初始化中断处理程序
	trap_init();
	irq_init();
	local_irq_enable();
	pci_init();
    disk_init();
    thread_init();
    timer_init();
	thread_start("thread_a",31,thread_a,NULL);
	
	//local_irq_enable();
	// local_irq_disable();
	
	// early_boot_irqs_disabled = true;
	printk("cpu = %d\n", cpu);

	while (1) {
		//time = csr_read64(LOONGARCH_CSR_TVAL);
		//printk("%lu\n",ticks);
        printk("m ");
	}
}
