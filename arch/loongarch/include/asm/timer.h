#ifndef __TIMER_H
#define __TIMER_H
#include <stdint.h>
#include <xkernel/list.h>
#include <asm/pt_regs.h>
#include <xkernel/sched.h>

/* seconds */
#define MSEC_PER_SEC	1000UL
#define USEC_PER_MSEC	1000UL
#define NSEC_PER_USEC	1000UL
#define NSEC_PER_MSEC	1000000UL
#define USEC_PER_SEC	1000000UL
#define NSEC_PER_SEC	1000000000L

/* clock and seconds */
#define CLK_FREQUENCY     100000000UL
#define CLK_PER_USEC      (CLK_FREQUENCY / USEC_PER_SEC)
#define NSEC_PER_CLK      (NSEC_PER_SEC  / CLK_FREQUENCY)
#define CLK_TO_SEC(clk)   ((clk) / CLK_FREQUENCY)
#define CLK_TO_USEC(clk)  ((clk) / CLK_PER_USEC)
#define USEC_TO_NSEC(us)  (us * NSEC_PER_USEC)

/* ticks and seconds */
#define CLK_PER_TICK       0x0400000UL
#define TICK_PER_SEC       (CLK_FREQUENCY / CLK_PER_TICK)
#define SEC_TO_TICK(sec)   (sec * TICK_PER_SEC)
#define NSEC_TO_TICK(nsec) (nsec / NSEC_PER_CLK / CLK_PER_TICK)

/* timer settings */
#define LVL_DEPTH        7
#define LVL_BITS         6
#define LVL_SIZE          (1UL << LVL_BITS)
#define LVL_MASK          (LVL_SIZE-1)
#define LVL_OFF(n)        ((n) * LVL_SIZE)

#define LVL_SHIFT(n)      ((n) * LVL_BITS)
#define LVL_START(n)      (1UL << LVL_SHIFT(n))

extern unsigned long ticks;
extern void intr_timer_handler(struct pt_regs *regs);

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
int sys_gettimeofday(struct timespec *ts);
long sys_times(struct tms *tms);
void utimes_begin(struct task_struct *pcb);
void utimes_end(struct task_struct *pcb);
void stimes_begin(struct task_struct *pcb);
void stimes_end(struct task_struct *pcb);
void add_timer(struct timer_list *timer);
int mod_timer(struct timer_list *timer,unsigned long expire);
int del_timer(struct timer_list *timer);

static inline uint64_t timespec2ticks(struct timespec *ts)
{
	uint64_t sec_ticks  = SEC_TO_TICK(ts->tv_sec);
	uint64_t nsec_ticks = NSEC_TO_TICK(ts->tv_nsec);
	return sec_ticks + nsec_ticks;
}

#endif
