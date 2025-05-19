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
//#include <xkernel/initcode.h>
#include <asm/timer.h>

extern void __init __no_sanitize_address start_kernel(void);

bool early_boot_irqs_disabled;
#define VMEM_SIZE (1UL << (9 + 9 + 12))
extern void disk_init(void);
extern void irq_init(void);
extern void setup_arch(void);
extern void trap_init(void);

#ifdef CONFIG_LOONGARCH
void thread_a(void *unused);
void thread_a(void *unused) {
    printk("enter thread_a\n");
    while(1)
        {
            printk("thread_a pid=%d\n", sys_getpid());
        }
}
#endif /* CONFIG_LOONGARCH */

#ifdef CONFIG_LOONGARCH
char *sstr = "hello\n";
void proc_1(void *unused) {
    while(1)
        {
            syscall(SYS_PSTR, sstr);
        }
}
#endif

void timer_func(unsigned long unused) {
    printk("timer done");
}

//char usrprog[2][40000];
int sysnums = 3;
//extern char* sysname[];

//char init_program[70000];
//char filename[50][64] = {0};

char xkernel_banner[] = "        __                              .__   \n"
                        "___  __|  | __ ___________  ____   ____ |  |  \n"
                        "\\  \\/  /  |/ // __ \\_  __ \\/    \\_/ __ \\|  |  \n"
                        " >    <|    <\\  ___/|  | \\/   |  \\  ___/|  |__\n"
                        "/__/\\_ \\__|_ \\___  >__|  |___|  /\\___  >____/\n"
                        "      \\/    \\/    \\/           \\/     \\/      ";
#ifndef CONFIG_RISCV
#endif
void sppppp(void) {
    asm volatile("li a0, 0x10000000\n\t"
                 "li a1, 'y'\n\t"
                 "sb a1,0(a0)\n\t"
                 "j .");
}

void console_putchar(uint8_t ch) {
    register uintptr_t a0 asm("x10") = (uintptr_t)ch;
    register uintptr_t a1 asm("x11") = 0;
    register uintptr_t a2 asm("x12") = 0;
    register uintptr_t a7 asm("x17") = 1; // which = 1 (SBI_CONSOLE_PUTCHAR)
    register uintptr_t ret asm("x10");

    asm volatile("ecall"
                 : "=r"(ret)
                 : "r"(a0), "r"(a1), "r"(a2), "r"(a7)
                 : "memory");
    (void)ret; // 防止未使用变量警告
}

void puts(const char *s) {
    while(*s)
        {
            console_putchar(*s);
            s++;
        }
}

void __init __no_sanitize_address start_kernel(void) {
    char str[] = "xkernel";
    console_putchar('h');

    puts("hello\n");
    printk("%s\n", xkernel_banner);
    // sppppp();
   
    #ifndef CONFIG_RISCV
    int cpu = smp_processor_id();

    local_irq_disable();

    printk("%s\n", xkernel_banner);
    printk("sizeof pcb:%x\n", sizeof(struct task_struct));

    pr_info("%s %s-%d.%d.%d\n", "hello", str, 0, 0, 1);
    setup_arch(); //初始化体系结构
    // mem_init();
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
    //proc_files_init();

    struct task_struct *cur  = running_thread();
    struct task_struct *bak  = bak_pcb(cur);
    early_boot_irqs_disabled = true;
    int fd                   = sys_open("initcode", O_CREATE | O_RDWR, 0);
    sys_chdir("/sdcard");
    if(fd == -1)
        {
            printk("open failed");
    }
    sys_write(fd, init_code, init_code_len);
    *cur = *bak;
    printk("last test..................\n");
    test_pcb();
    local_irq_enable();
#endif
    while(1)
        {
        }
}
