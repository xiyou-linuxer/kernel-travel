#include <asm/timer.h>
#include <xkernel/thread.h>
#include <debug.h>
#include <xkernel/printk.h>
#include <asm/pt_regs.h>
#include <asm/loongarch.h>
#include <trap/irq.h>
#include <trap/softirq.h>
#include <xkernel/stdio.h>
#include <xkernel/switch.h>

#define IRQ0_FREQUENCY     100
#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)


unsigned long ticks;
unsigned long timer_ticks;

static struct timer_vec tvecs[LVL_DEPTH];

static void tvecs_init(void)
{
	for (int i = 0 ; i < LVL_DEPTH ; i++)
		for (int j = 0 ; j < LVL_SIZE ; j++)
			list_init(&tvecs[i].vec[j]);
}

void intr_timer_handler(struct pt_regs *regs)
{
	struct task_struct* cur_thread = running_thread();

	ASSERT(cur_thread->stack_magic == STACK_MAGIC_NUM);

	cur_thread->elapsed_ticks++;
	ticks++;

	write_csr_ticlr(read_csr_ticlr() | (0x1 << 0));
	//printk("%d  ",cur_thread->ticks);
	//printk("%s : %d",cur_thread->name,cur_thread->ticks);

	if (cur_thread->ticks == 0) {
		//printk("time to schedule...");
		schedule();
	} else {
		cur_thread->ticks--;
	}
}

int sys_gettimeofday(struct timespec *ts)
{
	uint64_t clk = rdtime();
	uint64_t usec = CLK_TO_USEC(clk) % USEC_PER_SEC;

	ts->tv_sec  = CLK_TO_SEC(clk);
	ts->tv_nsec = USEC_TO_NSEC(usec);

	return 0;
}

void utimes_begin(struct task_struct *pcb)
{
	struct start_tms *start_times = &pcb->start_times;
	start_times->start_utime = rdtime();
}

void utimes_end(struct task_struct *pcb)
{
	long cur_clk    = rdtime();
	long start_clk  = pcb->start_times.start_utime;
	struct tms *rec = &pcb->time_record;
	rec->tms_utime += cur_clk - start_clk;
}

void stimes_begin(struct task_struct *pcb)
{
	struct start_tms *start_times = &pcb->start_times;
	start_times->start_stime = rdtime();
}

void stimes_end(struct task_struct *pcb)
{
	long cur_clk    = rdtime();
	long start_clk  = pcb->start_times.start_stime;
	struct tms *rec = &pcb->time_record;
	rec->tms_stime += cur_clk - start_clk;
}

long sys_times(struct tms *tms)
{
	struct task_struct *cur = running_thread();
	long ret_time = -1;
	utimes_end(cur);
	//stimes_end(cur);

	*tms = cur->time_record;

	utimes_begin(cur);
	stimes_end(cur);
	return ticks;
}

static inline int detach_timer(struct timer_list *timer)
{
	if (timer->elm.prev == NULL) return 0;
	list_remove(&timer->elm);
	return 1;
}

static inline unsigned int calc_index(unsigned long expire,unsigned int lvl)
{
	unsigned int lvl_vecidx = (expire >> LVL_SHIFT(lvl));  // vec[0]==NULL
	return (lvl_vecidx&LVL_MASK) + LVL_OFF(lvl);
}

static unsigned int calc_wheel_idx(struct timer_list *timer)
{
	unsigned int idx;
	unsigned long expire = timer->expires;
	unsigned long delta  = expire - timer_ticks;

	if (delta < LVL_START(1)) {
		idx = calc_index(expire,0);
	} else if (delta < LVL_START(2)) {
		idx = calc_index(expire,1);
	} else if (delta < LVL_START(3)) {
		idx = calc_index(expire,2);
	} else if (delta < LVL_START(4)) {
		idx = calc_index(expire,3);
	} else if (delta < LVL_START(5)) {
		idx = calc_index(expire,4);
	} else if (delta < LVL_START(6)) {
		idx = calc_index(expire,5);
	}

	return idx;
}

void enqueue_timer(struct timer_list *timer,unsigned int idx)
{
	list_append(&tvecs[idx/LVL_SIZE].vec[idx%LVL_SIZE],&timer->elm);
}

static void internal_add_timer(struct timer_list *timer)
{
	unsigned int idx = calc_wheel_idx(timer);
	enqueue_timer(timer,idx);
}

static void cascade_timers(struct timer_vec *tvec)
{
	struct list *vec_list = &tvec->vec[tvec->index];
	struct list_elem *cur = vec_list->head.next;

	//printk("   index=%d\n",tvec->index);
	struct timer_list *cur_timer;
	while (cur != &vec_list->tail)
	{
		cur_timer = elem2entry(struct timer_list,elm,cur);
		cur = cur->next;
	}
	list_init(vec_list);
	tvec->index = (tvec->index + 1)&LVL_MASK;
}

static void run_timers(void* unused)
{
	struct list *cur_list = &tvecs->vec[tvecs->index];
	//printk("run_timers..\n");
	while ((long)(ticks - timer_ticks) > 0)
	{
		struct timer_list *timer;
		if (!tvecs[0].index) {
			for (int n = 1 ; n < LVL_DEPTH ; n++) {
				//printk("level %d",n);
				cascade_timers(&tvecs[n]);
				if (tvecs[n].index != 1) break;
			}
		}
		while (!list_empty(cur_list)) {
			timer = elem2entry(struct timer_list,elm,cur_list->head.next);
			timer->func(timer->data);
			list_remove(cur_list->head.next);
		}
		timer_ticks++;
		tvecs[0].index = (tvecs[0].index + 1)&LVL_MASK;
	}
	raise_softirq(TIMER_SOFTIRQ);
}

void add_timer(struct timer_list *timer)
{
	if (timer->elm.prev != NULL) {
		printk("timer has been added\n");
		BUG();
	}
	internal_add_timer(timer);
}

int mod_timer(struct timer_list *timer,unsigned long expire)
{
	int ret = 0;
	timer->expires = expire;
	ret = detach_timer(timer);
	internal_add_timer(timer);
	return ret;
}

int del_timer(struct timer_list *timer)
{
	return detach_timer(timer);
}

void timer_init() {
	printk("timer_init start\n");

	register_handler(EXCCODE_TIMER, intr_timer_handler);
	tvecs_init();
	open_softirq(TIMER_SOFTIRQ,run_timers,NULL);
	raise_softirq(TIMER_SOFTIRQ);

	printk("timer_init done\n");
}

