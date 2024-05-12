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
#include <asm/page.h>
#include <asm/tlb.h>
#include <linux/thread.h>
#include <linux/ahci.h>
#include <linux/block_device.h>
#include <sync.h>
#include <process.h>
#include <linux/memory.h>
#include <linux/string.h>
#include <syscall_init.h>
#include <asm/syscall.h>
#include <asm/stdio.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <fs/cluster.h>
extern void __init __no_sanitize_address start_kernel(void);

bool early_boot_irqs_disabled;
#define VMEM_SIZE (1UL << (9 + 9 + 12))
extern void disk_init(void);
extern void irq_init(void);
extern void setup_arch(void);
extern void trap_init(void);

void thread_a(void *unused);
#ifdef CONFIG_LOONGARCH
#endif /* CONFIG_LOONGARCH */
void thread_a(void *unused)
{
	printk("enter thread_a\n");
	while (1) {
		printk("thread_a pid=%d\n",sys_getpid());
	}
}

char* sstr = "hello\n";
void proc_1(void *unused)
{
	while(1) {
		syscall(SYS_PSTR,sstr);
	}
}

void timer_func(unsigned long unused){
	printk("timer done");
}

char usrprog[90000];

void __init __no_sanitize_address start_kernel(void)
{
	char str[] = "xkernel";
	int cpu = smp_processor_id();
	printk("%lx\n", lalist_mem_map.map_count);
	printk("%lx\n", lalist_mem_map.map->mem_type);
	printk("%lx\n", lalist_mem_map.map->mem_start);
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
	//struct timer_list timer;
	//timer.elm.prev = timer.elm.next = NULL;
	//timer.expires = ticks + 100000;
	//timer.func = timer_func;
	//timer.data = 7;
	//add_timer(&timer);

	syscall_init();
	fs_init();
	//thread_start("thread_a",10,thread_a,NULL);
	//block_read(5,50,(uint64_t)usrprog,1);
	//process_execute(usrprog,"proc_1");
	Dirent *file;
	struct path_search_record searched_record;
	file = search_file("/getpid",&searched_record);
	filepnt_init(file);
	pre_read(file,(unsigned long)usrprog,file->file_size/4096+1);
	//printk("%s\n", buf);
	file_read(file, 0, (unsigned long)usrprog, 0, file->file_size);
	printk("%s\n", usrprog);
	process_execute(usrprog,"proc1");
	


	// early_boot_irqs_disabled = true;
	printk("cpu = %d\n", cpu);
	while (1) {
		//time = csr_read64(LOONGARCH_CSR_TVAL);
		//printk("%lu\n",ticks);
		//printk("main pid=%d\n ",sys_getpid());
	}
}
