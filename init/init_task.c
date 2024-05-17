#include <asm/thread_info.h>
#include <asm/cache.h>
#include <xkernel/init_task.h>
#include <xkernel/sched.h>
#include <xkernel/compiler_attributes.h>

struct task_struct init_task
	__aligned(L1_CACHE_BYTES)
= {
	// .__state = 0,
	// .stack = init_stack,
};

#ifndef CONFIG_THREAD_INFO_IN_TASK
struct thread_info init_thread_info __init_thread_info = INIT_THREAD_INFO(init_task);
#endif
