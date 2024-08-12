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
#include <fs/syscall_fs.h>
#include <xkernel/memory.h>
#include <xkernel/sigset.h>

struct task_struct* main_thread;
struct task_struct* idle_thread;
struct list thread_ready_list;
struct list thread_all_list;

struct list_elem* thread_tag;

extern void irq_exit(void);
extern void irq_enter(void);
extern void bh_enable(void);
bool switching;

uint8_t pid_bitmap_bits[128] = {0};

struct pid_pool {
	struct bitmap pid_bitmap;
	uint32_t pid_start;
	struct lock pid_lock;
}pid_pool;

void test_pcb(void)
{
	struct task_struct *pcb = running_thread();
	printk("name:%s\n",pcb->name);
	printk("self_kstack:%llx\n",pcb->self_kstack);
	printk("thread_func :%llx\n",pcb->function);
	printk("func_arg:%llx\n",pcb->func_arg);
	printk("time_record.stime:%d\n",pcb->time_record.tms_stime);
	printk("ppid:%d\n",pcb->ppid);
	printk("pid:%d\n",pcb->pid);
	printk("clear_child_tid:%llx\n",pcb->clear_child_tid);
	printk("task_status:%llx\n",pcb->status);
	printk("priority:%llx\n",pcb->priority);
	printk("ticks:%d\n",pcb->ticks);
	printk("elapsed_ticks:%d\n",pcb->elapsed_ticks);
	//printk("pending:%d\n",pcb->pending.signal);
	printk("userprog_vaddr.btmp:%llx  vaddr_start:%llx:\n",pcb->usrprog_vaddr.btmp,pcb->usrprog_vaddr.vaddr_start);
	printk("general_tag.next:%llx prev:%llx\n",pcb->general_tag.next,pcb->general_tag.prev);
	printk("fd_table: ");
	for (int i=0;i<10;i++) printk("%d ",pcb->fd_table[i]);
	printk("cwd:%s\n",pcb->cwd);
	printk("cwd_dirent:%llx\n",pcb->cwd_dirent);
	printk("all_list_tag.next:%llx prev:%llx\n",pcb->all_list_tag.next,pcb->all_list_tag.prev);
	printk("pgdir:%llx\n",pcb->pgdir);
	printk("asid:%llx\n",pcb->asid);
	printk("mm:%llx\n",pcb->mm);
	printk("magic:%llx\n",pcb->stack_magic);
}

struct task_struct *bak_pcb(struct task_struct *p)
{
	struct task_struct *bak = (struct task_struct*)get_page();
	*bak = *p;
	return bak;
}


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

pid_t sys_getpid(void)
{
	struct task_struct* cur = running_thread();
	return cur->pid;
}

pid_t sys_getppid(void)
{
	struct task_struct* cur = running_thread();
	return cur->ppid;
}

unsigned int sys_getgid(void)
{
	return 0;
}

pid_t sys_set_tid_address(int* tidptr)
{
	struct task_struct* cur = running_thread();
	cur->clear_child_tid = (uint64_t)tidptr;
	sys_chdir("/sdcard");
	return sys_getpid();
}

static int checkpid(struct list_elem* pelm,void* id)
{
	pid_t pid = (int64_t)id;
	struct task_struct* pcb = elem2entry(struct task_struct,all_list_tag,pelm);
	if (pcb->pid == pid) {
		return true;
	}
	return false;
}

struct task_struct* pid2thread(int64_t pid)
{
	struct list_elem* elm = list_traversal(&thread_all_list,checkpid,(void*)pid);
	if (elm == NULL) {
		return NULL;
	}
	return elem2entry(struct task_struct,all_list_tag,elm);
}

static void kernel_thread(void)
{
	printk("kernel_thread...\n");
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

struct task_struct* running_thread(void)
{
	register uint64_t sp asm("sp");
	//printk("now sp at:%x\n",sp);
	return (struct task_struct *)((sp-1) & ~(KERNEL_STACK_SIZE - 1));
}

void init_thread(struct task_struct *pthread, char *name, int prio)
{
	memset(pthread, 0, sizeof(*pthread));
	strcpy(pthread->name, name);
	pthread->pid  = allocate_pid();
	pthread->ppid = -1;

	if (pthread == main_thread) {
		pthread->status = TASK_RUNNING;
	} else {
		pthread->status = TASK_READY;
	}
	pthread->self_kstack = (uint64_t*)((uint64_t)pthread + KERNEL_STACK_SIZE);
	pthread->priority = prio;
	pthread->ticks = prio;
	pthread->elapsed_ticks = 0;
	init_sigset(&pthread->pending.signal);
	init_sigset(&pthread->blocked);
	init_handlers(&pthread->handlers);
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
	pthread->mm = (struct mm_struct*)get_page();
	mm_struct_init(pthread->mm);

	if (!pthread->mm) {
		goto error;
	}
	return;
error:
	// __free_pages_ok((struct page *)pthread->mm, 0, 0);
	printk("[Error] : 没有足够内存为 mm_struct 分配。");
	return;
}

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
	pthread->self_kstack = (uint64_t*)((uint64_t)pthread->self_kstack - sizeof(struct pt_regs));
	pthread->function = function;
	pthread->func_arg = func_arg;

	struct thread_struct *kthread_stack = &pthread->thread;
	memset(kthread_stack, 0, sizeof(struct thread_struct));
	kthread_stack->reg01 = (uint64_t)kernel_thread;
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
	//printk("schedule...\n");
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
	//printk("next:%s\n",next->name);


	utimes_begin(next);
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
	irq_enter();
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
	bh_enable();
	thread_preempt(thread);
}

//static int snap(uint64_t sleep_ticks)
//{
//	uint64_t expire = ticks + sleep_ticks;
//	intr_enable();
//	while (ticks < expire) {
//		printk("ticks:%d expire:%d\n",ticks,expire);
//	}
//	return (ticks-expire>=0 ? 0 : -1);
//}

static int snap(struct timespec *req)
{
	struct timespec cur_timer;
	struct timespec end_timer;
	sys_gettimeofday(&cur_timer);
	uint64_t nsec = cur_timer.tv_nsec + req->tv_nsec;
	end_timer.tv_sec = cur_timer.tv_sec + req->tv_sec + nsec/NSEC_PER_SEC;
	end_timer.tv_nsec = nsec % NSEC_PER_SEC;
	while (cur_timer.tv_sec < end_timer.tv_sec || (cur_timer.tv_sec==end_timer.tv_sec && cur_timer.tv_nsec < end_timer.tv_nsec)) {
		sys_gettimeofday(&cur_timer);
	}
	return 0;
}

static int normal_sleep(uint64_t sleep_ticks)
{
	struct timer_list* t = (struct timer_list*)get_page();
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

int sys_sleep(struct timespec *req,struct timespec *rem)
{
	uint64_t sleep_ticks = timespec2ticks(req);
	if (sleep_ticks >= 20) {
		return normal_sleep(sleep_ticks);
	}
	return snap(req);
}

void thread_exit(struct task_struct* exit)
{
	intr_disable();
	exit->status = TASK_DIED;
	if (elem_find(&thread_ready_list,&exit->general_tag)) {
		printk("find zombie in thread_ready_list");
		BUG();
	}
	list_remove(&exit->all_list_tag);

	if (exit != main_thread) {
		task_free(exit);
	}

	release_pid(exit->pid);
}

void thread_init(void)
{
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	pid_pool_init();

	process_execute("initcode","init",15);
	idle_thread = thread_start("idle",10,idle,NULL);
	make_main_thread();
}

