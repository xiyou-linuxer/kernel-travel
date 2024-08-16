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
#include <xkernel/mmap.h>
#include <xkernel/bitops.h>
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
#include <fs/fd.h>
#include <fs/buf.h>
#include <fs/syscall_fs.h>
#include <xkernel/initcode.h>
#include <asm/timer.h>

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

//char usrprog[2][40000];
int sysnums = 3;
//extern char* sysname[];

//char init_program[70000];
//char filename[50][64] = {0};

char xkernel_banner[] = \
"        __                              .__   \n"\
"___  __|  | __ ___________  ____   ____ |  |  \n"\
"\\  \\/  /  |/ // __ \\_  __ \\/    \\_/ __ \\|  |  \n"\
" >    <|    <\\  ___/|  | \\/   |  \\  ___/|  |__\n"\
"/__/\\_ \\__|_ \\___  >__|  |___|  /\\___  >____/\n"\
"      \\/    \\/    \\/           \\/     \\/      ";

void __init __no_sanitize_address start_kernel(void)
{
	char str[] = "xkernel";
	int cpu = smp_processor_id();
	
	local_irq_disable();

	printk("%s\n", xkernel_banner);
	printk("sizeof pcb:%x\n",sizeof(struct task_struct));

	pr_info("%s %s-%d.%d.%d\n", "hello", str, 0, 0, 1);
	setup_arch();//初始化体系结构
	mem_init();
	trap_init();
	irq_init();

	thread_init();
	test_pcb();
	timer_init();
	pci_init();
	console_init();
	disk_init();
	console_init();
	syscall_init();
	vfs_init();
	fs_init();

	struct task_struct *cur = running_thread();
	struct task_struct* bak = bak_pcb(cur);
	early_boot_irqs_disabled = true;
	int fd = sys_open("initcode",O_CREATE|O_RDWR,0);
	sys_chdir("/sdcard");
	if (fd == -1) {
		printk("open failed");
	}
	sys_write(fd,init_code,init_code_len);
	*cur = *bak;
	printk("last test..................\n");
	test_pcb();
	local_irq_enable();
	
	while (1) {

	}
}
