#include <xkernel/irqflags.h>
#include <xkernel/printk.h>
#include <xkernel/task.h>
#include <xkernel/init.h>
#include <xkernel/ns16550a.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <xkernel/smp.h>
#include <xkernel/thread.h>
#include <xkernel/ahci.h>
#include <xkernel/block_device.h>
#include <xkernel/console.h>
#include <xkernel/memory.h>
#include <xkernel/string.h>
#include <asm-generic/bitsperlong.h>
#include <trap/irq.h>
#include <asm/pci.h>
#include <asm/setup.h>
#include <asm/bootinfo.h>
#include <asm/boot_param.h>
#include <asm/timer.h>
#include <asm/page.h>
#include <asm/tlb.h>
#include <sync.h>
#include <process.h>
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

char usrprog[2][40000];
int sysnums = 3;
extern char* sysname[];

void __init __no_sanitize_address start_kernel(void)
{
	char str[] = "xkernel";
	int cpu = smp_processor_id();
	// serial_ns16550a_init(9600);
	printk("%s %s-%d.%d.%d\n", "hello", str, 0, 0, 1);
	setup_arch();//初始化体系结构
	mem_init();
	/*__alloc_pages for test*/
	// struct page *page = __alloc_pages(0, 7, 0);
	// struct page * page2 = __alloc_pages(0, 7, 0);
	// __free_pages_ok(page, 7, 0);
	// __free_pages_ok(page2, 7, 0);
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
	console_init();
	syscall_init();
	fs_init();
	//thread_start("thread_a",10,thread_a,NULL);
	//
	
	int count=0;
	char filename[9][128] = {0};
	for (int i = 0; i < NR_SYSCALLS; i++)
	{
		if (sysname[i] == NULL)
			continue;
		Dirent *file;
		struct path_search_record searched_record;
		int pri=5;
		strcpy(filename[count],"/");
		strcat(filename[count],sysname[i]);
		process_execute(filename[count],filename[count],pri);
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
