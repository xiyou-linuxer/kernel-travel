#ifndef __TIMER_H
#define __TIMER_H
#include <stdint.h>
#include <linux/list.h>
#include <asm/pt_regs.h>

#define MSEC_PER_SEC	1000UL
#define USEC_PER_MSEC	1000UL
#define NSEC_PER_USEC	1000UL
#define NSEC_PER_MSEC	1000000UL
#define USEC_PER_SEC	1000000UL
#define NSEC_PER_SEC	1000000000L

#define CLK_FREQUENCY    100000000
#define CLK_PER_USEC     (CLK_FREQUENCY / USEC_PER_SEC)
#define CLK_TO_SEC(clk)  ((clk) / CLK_FREQUENCY)
#define CLK_TO_USEC(clk) ((clk) / CLK_PER_USEC)
#define USEC_TO_NSEC(us) (us * NSEC_PER_USEC)

extern unsigned long ticks;
extern void intr_timer_handler(struct pt_regs *regs);

#define LVL_DEPTH       7
#define LVL_BITS        6
#define LVL_SIZE         (1UL << LVL_BITS)
#define LVL_MASK         (LVL_SIZE-1)
#define LVL_OFF(n)       ((n) * LVL_SIZE)

#define LVL_SHIFT(n)     ((n) * LVL_BITS)
#define LVL_START(n)     (1UL << LVL_SHIFT(n))

static inline uint64_t rdtime(void)
{
	int      tid = 0;
	uint64_t val = 0;

	asm volatile(
		"rdtime.d %0, %1 \n\t"
		: "=r"(val), "=r"(tid)
		:
		);
	return val;
}

struct timespec {
	time_t tv_sec;        /* 秒 */
	long   tv_nsec;       /* 纳秒, 范围在0~999999999 */
};

struct timer_vec {
	unsigned int index;
	struct list vec[LVL_SIZE];
};

struct timer_list {
	struct list_elem elm;
	unsigned long expires;
	void (*func)(unsigned long data);
	unsigned long data;
};

void timer_init(void);
int64_t sys_gettimeofday(struct timespec *ts);
void add_timer(struct timer_list *timer);
int mod_timer(struct timer_list *timer,unsigned long expire);
int del_timer(struct timer_list *timer);

#endif
