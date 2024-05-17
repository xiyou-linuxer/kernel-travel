#include <xkernel/thread.h>
#include <debug.h>
#include <bitmap.h>
#include <xkernel/printk.h>
#include <asm/bootinfo.h>
#include <xkernel/string.h>
#include <xkernel/stdio.h>
#include <asm/pt_regs.h>
#include <trap/irq.h>
#include <asm/loongarch.h>
#include <xkernel/switch.h>
#include <asm/asm-offsets.h>
#include <xkernel/sched.h>
#include <sync.h>
#include <allocator.h>
#include <process.h>
#include <trap/irq.h>
#include <asm/timer.h>

struct task_struct* main_thread;
struct task_struct* idle_thread;
struct list thread_ready_list;
struct list thread_all_list;

struct list_elem* thread_tag;

extern void irq_exit(void);
extern void irq_enter(void);
bool switching;

uint8_t pid_bitmap_bits[128] = {0};

struct pid_pool {
   struct bitmap pid_bitmap;
   uint32_t pid_start;
   struct lock pid_lock;
}pid_pool;

static void pid_pool_init(void) { 
   pid_pool.pid_start = 1;
   pid_pool.pid_bitmap.bits = pid_bitmap_bits;
   pid_pool.pid_bitmap.btmp_bytes_len = 128;
   bitmap_init(&pid_pool.pid_bitmap);
   lock_init(&pid_pool.pid_lock);
}

static pid_t allocate_pid(void) {
   lock_acquire(&pid_pool.pid_lock);
   int32_t bit_idx = bit_scan(&pid_pool.pid_bitmap, 1);
   bitmap_set(&pid_pool.pid_bitmap, bit_idx, 1);
   lock_release(&pid_pool.pid_lock);
   return (bit_idx + pid_pool.pid_start);
}

pid_t fork_pid(void) {
	return allocate_pid();
}

void release_pid(pid_t pid) {
   lock_acquire(&pid_pool.pid_lock);
   int32_t bit_idx = pid - pid_pool.pid_start;
   bitmap_set(&pid_pool.pid_bitmap, bit_idx, 0);
   lock_release(&pid_pool.pid_lock);
}

static void kernel_thread(void)
{
    printk("kernel_thread...");
    struct task_struct *task = running_thread();
    intr_enable();
    task->function(task->func_arg);
    return;
}

static void idle(void* arg)
{
	while (1) {
		thread_block(TASK_BLOCKED);
		intr_enable();
		asm volatile("idle 0");
	}
}

struct task_struct* running_thread()
{
    register uint64_t sp asm("sp");
    //printk("now sp at:%x\n",sp);
    return (struct task_struct *)((sp-1) & ~(KERNEL_STACK_SIZE - 1));
}

void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->pid = allocate_pid();

    if (pthread == main_thread) {
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }
    pthread->self_kstack = (uint64_t*)((uint64_t)pthread + KERNEL_STACK_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = 0;
    pthread->fd_table[0] = 0;
    pthread->fd_table[1] = 1;
    pthread->fd_table[2] = 2;
    uint8_t fd_idx = 3;
    while (fd_idx < MAX_FILES_OPEN_PER_PROC) {
        pthread->fd_table[fd_idx] = -1;
        fd_idx++;
    }
    pthread->cwd[0] = '/';
    pthread->cwd_dirent = NULL;
    pthread->stack_magic = STACK_MAGIC_NUM;
}

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    pthread->self_kstack = (uint64_t*)((uint64_t)pthread->self_kstack - sizeof(struct pt_regs));
    pthread->function = function;
    pthread->func_arg = func_arg;

    struct thread_struct *kthread_stack = &pthread->thread;
    memset(kthread_stack, 0, sizeof(struct thread_struct));
    kthread_stack->reg01 = (uint64_t)kernel_thread;
    printk("thread reg01=%x\n",kthread_stack->reg01);
    kthread_stack->csr_crmd = read_csr_crmd();
    kthread_stack->csr_prmd = read_csr_prmd();
    kthread_stack->reg03 = (uint64_t)pthread->self_kstack;
}

struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg)
{
    struct task_struct *thread = task_alloc();
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    return thread;
}

static void make_main_thread(void)
{
    main_thread = running_thread();
    init_thread(main_thread, "main", 10);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule()
{
	printk("schedule...\n");
	ASSERT(intr_get_status() == INTR_OFF);

	struct task_struct* cur = running_thread();
	if (cur->status == TASK_RUNNING) {
		ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	} else {

	}

	if (list_empty(&thread_ready_list)) {
		thread_unblock(idle_thread);
	}

	thread_tag = NULL;	  // thread_tag清空
	thread_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
	page_dir_activate(next);
	next->status = TASK_RUNNING;
	//printk("curticks:%d\n",cur->ticks);
	//printk("next:%s",next->name);

	irq_exit();
	switching = 1;
	switch_to(cur, next);
}

void thread_block(enum task_status stat)
{
	enum intr_status old_status = intr_disable();
	ASSERT(stat==TASK_BLOCKED || \
		   stat==TASK_WAITING || \
		   stat==TASK_HANGING );

	struct task_struct* cur = (struct task_struct*)running_thread();
	ASSERT(cur->status==TASK_RUNNING);
	cur->status = stat;
	schedule();
	intr_set_status(old_status);
}

void thread_unblock(struct task_struct *thread)
{
	enum intr_status old_status = intr_disable();
	ASSERT(thread->status==TASK_BLOCKED || \
		   thread->status==TASK_WAITING || \
		   thread->status==TASK_HANGING );

	thread->status = TASK_READY;
	if (elem_find(&thread_ready_list,&thread->general_tag)) {
		BUG();
	}
	list_push(&thread_ready_list,&thread->general_tag);
	intr_set_status(old_status);
}

void thread_preempt(struct task_struct *thread)
{
	thread_unblock(thread);
	intr_disable();
	irq_enter();
	schedule();
}

void thread_yield(void) {
	struct task_struct* cur = running_thread();
	enum intr_status old_status = intr_disable();
	ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
	list_append(&thread_ready_list, &cur->general_tag);
	cur->status = TASK_READY;
	schedule();
	intr_set_status(old_status);
}

static void wake_sleep(unsigned long sleeping_thread)
{
	struct task_struct *thread = (struct task_struct*)sleeping_thread;
	thread_preempt(thread);
}

int sys_sleep(struct timespec *req,struct timespec *rem)
{
	struct timer_list* t = (struct timer_list*)get_page();
	uint64_t sleep_ticks = timespec2ticks(req);
	printk("sleep_ticks=%d\n",sleep_ticks);
	uint64_t expire      = ticks + sleep_ticks;
	struct task_struct *cur = running_thread();
	t->expires = expire;
	t->func    = wake_sleep;
	t->data    = (unsigned long)running_thread();
	t->elm.prev = t->elm.next = NULL;
	add_timer(t);
	thread_block(TASK_BLOCKED);

	return (ticks-expire>=0 ? 0 : -1);
}

void thread_init(void)
{
	printk("thread_init start\n");

	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	pid_pool_init();

	idle_thread = thread_start("idle",10,idle,NULL);
	make_main_thread();

	printk("thread_init done\n");
}

