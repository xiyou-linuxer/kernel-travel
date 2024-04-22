#ifndef __KERNEL_ALLOCATOR_H
#define __KERNEL_ALLOCATOR_H
#include <thread.h>
#include <asm/bootinfo.h>
#include <linux/compiler_attributes.h>

union task_page {
    struct task_struct task;
    char padding[KERNEL_STACK_SIZE];
} __aligned(KERNEL_STACK_SIZE);

struct user_stack_page {
    char padding[KERNEL_STACK_SIZE];
}__aligned(KERNEL_STACK_SIZE);

struct task_struct *task_alloc(void);
void* userstk_alloc(void);

#endif
