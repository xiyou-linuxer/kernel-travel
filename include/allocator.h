#ifndef __KERNEL_ALLOCATOR_H
#define __KERNEL_ALLOCATOR_H
#include <xkernel/thread.h>
#include <asm/bootinfo.h>
#include <xkernel/compiler_attributes.h>

#define USER_STACK ((1 << (9+9+12)))

union task_page {
    struct task_struct task;
    char padding[KERNEL_STACK_SIZE];
} __aligned(KERNEL_STACK_SIZE);

struct user_stack_page {
    char padding[KERNEL_STACK_SIZE];
}__aligned(KERNEL_STACK_SIZE);

struct task_struct *task_alloc(void);
void task_free(struct task_struct* task);
void* userstk_alloc(uint64_t pdir);

#endif
