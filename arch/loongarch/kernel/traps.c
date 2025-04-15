#include <asm/setup.h>
#include <asm/loongarch.h>
#include <asm/pt_regs.h>
#include <asm/cacheflush.h>
#include <xkernel/printk.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <signal.h>
#include <xkernel/thread.h>

extern void *vector_table[];
extern void tlb_handler(void);
extern void syscall_handler(void);
extern void do_irq(struct pt_regs *regs, uint64_t virq);

extern void handle_reserved(void);

#define INTCLR_0 0x1fe0142c
#define INTCLR_1 0x1fe0146c
#define INTPOL_0 0x1fe01430
#define INTPOL_1 0x1fe01470
#define INTEDGE_0 0x1fe01434
#define INTEDGE_1 0x1fe01474
#define BOUNCE_0 0x1fe01438
#define BOUNCE_1 0x1fe01478
#define AUTO_0 0x1fe0143c
#define AUTO_1 0x1fe0147c

void handle_reserved(void)
{
	printk("have a exception happened\n");
	while (1) ;
}

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int fls(int x)
{
	return x ? sizeof(x) * 8 - __builtin_clz(x) : 0;
}

extern int __ilog2_u32(unsigned int n);
int __ilog2_u32(unsigned int n)
{
	return fls(n) - 1;
}

/**
 * setup_vint_size - 设置普通例外与中断处理程序的间距
 * @size: 间距的大小(字节为单位)
 */
static inline void setup_vint_size(unsigned int size)
{
	unsigned int vs;

	/**
	 * 一条指令4字节(32位)
	 * size / 4 = 例外与中断处理程序的指令数
	 * vs = 以2为底，指令数的对数
	 */
	vs = __ilog2_u32(size / 4);

	/**
	 * CSR_ECFG寄存器的VS域为3bit，最大为7
	 * 为0时含义不同，即普通例外与中断处理程序的入口相同
	 */
	if (vs == 0 || vs > 7) {
		printk("BUG: vint_size %d Not support yet\n", vs);
		while(1);
	}

	/**
	 * 设置例外与中断处理程序的间距为2^vs条指令
	 */
	csr_xchg32(vs << CSR_ECFG_VS_SHIFT, CSR_ECFG_VS, LOONGARCH_CSR_ECFG);
}

/**
 * hwirq_to_virq - 硬件中断号转换为虚拟中断号
 * @hwirq: 硬件中断号
 */
static unsigned long hwirq_to_virq(unsigned long hwirq)
{
	return EXCCODE_INT_START + hwirq;
}

extern void before_ret(struct pt_regs *regs);
void before_ret(struct pt_regs *regs)
{
	if (running_thread() == main_thread)
		return ;
	//printk("before_ret:%llx\n",regs);
	check_signals(regs);
}

/**
 * do_vint - 中断处理入口程序（C语言部分）
 * @regs: 指向中断栈内容
 * @sp: 中断栈指针
 * regs == sp
 */
extern void do_vint(struct pt_regs *regs, unsigned long sp);
void do_vint(struct pt_regs *regs, unsigned long sp)
{
	unsigned long hwirq = *(unsigned long *)regs;
	unsigned long virq;
	//printk("hwirq %x\n",hwirq);
	virq = hwirq_to_virq(hwirq);
	do_irq(regs, virq);
}

#define SZ_64K		0x00010000

/**
 * 普通例外与中断处理程序入口
 */
unsigned long eentry;
/**
 * tlb重填例外处理程序入口
 */
unsigned long tlbrentry;

long exception_handlers[VECSIZE * 128 / sizeof(long)] __attribute__((aligned(SZ_64K)));

/**
 * configure_exception_vector - 配置例外与中断处理程序入口
 */
static void configure_exception_vector(void)
{
	eentry = (unsigned long)exception_handlers;
	tlbrentry = (unsigned long)exception_handlers + 80 * VECSIZE;

	/**
	 * 设置普通例外与中断处理程序入口
	 */
	csr_write64(eentry, LOONGARCH_CSR_EENTRY);
	/**
	 * 设置机器错误例外处理程序入口
	 */
	csr_write64(eentry, LOONGARCH_CSR_MERRENTRY);
	/**
	 * 设置tlb重填例外处理程序入口
	 */
	csr_write64(tlbrentry, LOONGARCH_CSR_TLBRENTRY);
}

void per_cpu_trap_init(int cpu)
{
	unsigned int i;
	/**
	 * 设置例外与中断处理程序的间距
	 */
	setup_vint_size(VECSIZE);
	/**
	 * 配置例外与中断处理程序入口
	 */
	configure_exception_vector();

	/**
	 * 初始化例外处理程序
	 */
	if (cpu == 0)
		for (i = 0; i < 64; i++)
			set_handler(i * VECSIZE, handle_reserved, VECSIZE);

	// tlb_init(cpu);
}

/**
 * set_handler - 设置例外处理程序句柄
 *
 * @offset: 相对于例外处理程序入口地址的偏移量
 * @addr: 例外处理程序地址
 * @size: 例外处理程序大小
 */
void set_handler(unsigned long offset, void *addr, unsigned long size)
{
	memcpy((void *)eentry + offset, addr, size);
	local_flush_icache_range(eentry + offset, eentry + offset + size);
}

/*初始化io中断环境
*/
static void io_irq_init(void)
{
	/*clr*/
	*(unsigned int*)(CSR_DMW0_BASE | INTCLR_0) = ~0x0U;
	*(unsigned int*)(CSR_DMW0_BASE | INTCLR_1) = ~0x0U;
	/*pol*/
	*(unsigned int*)(CSR_DMW0_BASE | INTPOL_0) = 0x0U;
	*(unsigned int*)(CSR_DMW0_BASE | INTPOL_1) = 0x0U;
	/*edge*/
	*(unsigned int*)(CSR_DMW0_BASE | INTEDGE_0) = 0x0U;
	*(unsigned int*)(CSR_DMW0_BASE | INTEDGE_1) = 0x0U;
	/*bou*/
	*(unsigned int*)(CSR_DMW0_BASE | BOUNCE_0) = 0x0U;
	*(unsigned int*)(CSR_DMW0_BASE | BOUNCE_1) = 0x0U;
	/*auto*/
	*(unsigned int*)(CSR_DMW0_BASE | AUTO_0) = 0x0U;
	*(unsigned int*)(CSR_DMW0_BASE | AUTO_1) = 0x0U;
}
/**
 * trap_init - 例外与中断处理初始化
 */
extern void trap_init(void);
void trap_init(void)
{
	unsigned long i;
	void *vector_start;
	unsigned long tcfg = 0x0400000UL | (1U << 0) | (1U << 1);
	unsigned long ecfg;

	/**
	 * 清空中断状态
	 */
	clear_csr_ecfg(ECFG0_IM);
	clear_csr_estat(ESTATF_IP);

	/**
	 * 初始化中断处理入口程序
	 */
	for (i = EXCCODE_INT_START; i < EXCCODE_INT_END; i++) {
		vector_start = vector_table[i - EXCCODE_INT_START];
		set_handler(i * VECSIZE, vector_start, VECSIZE);
	}
	set_handler(80*VECSIZE,tlb_handler,VECSIZE);
	set_handler(EXCCODE_SYS * VECSIZE,syscall_handler,VECSIZE);

	local_flush_icache_range(eentry, eentry + 0x400);
	//初始化定时器
	write_csr_tcfg(tcfg);
	#ifdef CONFIG_VIRT

	#else ifdef CONFIG_2K1000LA

	io_irq_init();

	#endif
	change_csr_ecfg(CSR_ECFG_IM, ecfg | 0x1 << 11);
}
