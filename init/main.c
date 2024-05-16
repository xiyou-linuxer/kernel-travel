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
#include <linux/console.h>
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

char usrprog[3][50000];
int sysnums = 3;
extern char* sysname[];

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
	console_init();
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
	//
	
	int count=0;
	for (int i = 0; i < NR_SYSCALLS; i++)
	{
		if (sysname[i] == NULL)
			continue;
		Dirent *file;
		struct path_search_record searched_record;
		int pri=5;
		char filename[128] = {0};
		strcpy(filename,"/");
		strcat(filename,sysname[i]);
		//if (strcmp(filename,"/sleep"))
		//	pri=40;
		file = search_file(filename,&searched_record);
		filepnt_init(file);
		pre_read(file,(unsigned long)usrprog[count],file->file_size/4096+1);
		file_read(file, 0, (unsigned long)usrprog[count], 0, file->file_size);
		//printk("%s\n", usrprog);
		process_execute(usrprog[count],filename,pri);
		count++;
	}

	// early_boot_irqs_disabled = true;
	printk("cpu = %d\n", cpu);
	//struct timespec ts;
	struct timespec req;
	req.tv_sec=1;req.tv_nsec=0;
	while (1) {
		//sys_gettimeofday(&ts);
		//printk("now %ds:%dns\n",ts.tv_sec,ts.tv_nsec);
		//sys_sleep(&req,&req);
		//unsigned long time = csr_read64(LOONGARCH_CSR_TVAL);
		//printk("%llx  ",time);
		//printk("main pid=%d\n ",sys_getpid());
	}
}
