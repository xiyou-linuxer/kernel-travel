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

char xkernel_banner[] = \
"        __                              .__   \n"\
"___  __|  | __ ___________  ____   ____ |  |  \n"\
"\\  \\/  /  |/ // __ \\_  __ \\/    \\_/ __ \\|  |  \n"\
" >    <|    <\\  ___/|  | \\/   |  \\  ___/|  |__\n"\
"/__/\\_ \\__|_ \\___  >__|  |___|  /\\___  >____/\n"\
"      \\/    \\/    \\/           \\/     \\/      ";

#define UART_BASE 0x10000000
#define UART_THR (volatile char*)(UART_BASE + 0x00) // Transmit Holding Register

static inline void putc(char c) {
    *UART_THR = c;
}
void __init __no_sanitize_address start_kernel(void)
{

	asm volatile("li a0, 0x10000000\n\t"
		"li a1, 'P'\n\t"
		"sb a1,0(a0)\n\t"
		);
	// putc('H');
    // putc('e');
    // putc('l');
    // putc('l');
    // putc('o');
	while (1) {

	}
}
