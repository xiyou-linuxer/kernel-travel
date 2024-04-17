#include <thread.h>
#include <debug.h>
#include <bitmap.h>
#include <linux/printk.h>
#include <asm/bootinfo.h>
#include <linux/string.h>
#include <linux/stdio.h>
#include <asm/pt_regs.h>
#include <trap/irq.h>
#include <asm/loongarch.h>
#include <switch.h>
#include <asm/asm-offsets.h>

struct task_struct* main_thread;
struct task_struct* idle_thread;
struct list thread_ready_list;
struct list thread_all_list;

struct list_elem* thread_tag;

/*
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
*/

static void kernel_thread(void)
{
    printk("kernel_thread...");
    struct task_struct *task = running_thread();
    intr_enable();
    task->function(task->func_arg);
    return;
}

struct task_struct_allocator_t {
    struct task_struct tasks[256];
    bool used[256];
} task_struct_allocator = {
    .tasks = { 0 },
    .used = { 0 },
};

struct task_struct *task_alloc(void)
{
    struct task_struct *task = NULL;
    int i;

    for (i = 0 ; i < 256 ; i++) {
        if (task_struct_allocator.used[i] == false) {
            task = &task_struct_allocator.tasks[i];
            task_struct_allocator.used[i] = true;
            break;
        }
    }

    return task;
}

struct task_struct* running_thread()
{
    register uint64_t sp asm("sp");
    printk("now sp at:%x\n",sp);
    return (struct task_struct *)(sp & ~(KERNEL_STACK_SIZE - 1));
}

void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);

    if (pthread == main_thread) {
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }

    pthread->self_kstack = (uint64_t *)((uint64_t)pthread + KERNEL_STACK_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->stack_magic = STACK_MAGIC_NUM;
}

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    pthread->self_kstack -= sizeof(struct pt_regs);
    pthread->function = function;
    pthread->func_arg = func_arg;

    struct thread_stack *kthread_stack = &pthread->thread;
    memset(kthread_stack, 0, sizeof(struct thread_stack));
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

    //printk("thread start thread reg01:%x\n",thread->thread.reg01);
    //printk("thread reg01 at:%x\n",(unsigned long)&thread->thread.reg01);
    //printk("thread+THREAD_REG01=%x\n",(unsigned long)thread+THREAD_REG01);
    switch_to(running_thread(),thread);

    return thread;
}


static void make_main_thread(void)
{
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void thread_init(void)
{
    printk("thread_init start\n");

    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    //pid_pool_init();

    make_main_thread();

    printk("thread_init done\n");
}

