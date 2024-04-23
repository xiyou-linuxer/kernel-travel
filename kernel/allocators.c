#include <allocator.h>

struct task_struct_allocator_t {
    union task_page task_pages[256];
    bool used[256];
} task_struct_allocator = {
    .task_pages = { 0 },
    .used = { 0 },
};

struct user_stack_allocator_t {
    struct user_stack_page stk_pages[16];
    bool used[16];
} user_stack_allocator = {
    .stk_pages = {0},
    .used = {0},
};

struct task_struct *task_alloc(void)
{
    struct task_struct *task = NULL;
    int i;

    for (i = 0 ; i < 256 ; i++) {
        if (task_struct_allocator.used[i] == false) {
            task = &task_struct_allocator.task_pages[i].task;
            task_struct_allocator.used[i] = true;
            break;
        }
    }

    return task;
}


void* userstk_alloc(void)
{
    void *stk_ptr = NULL;
    int i;

    for (i = 0 ; i < 16 ; i++) {
        if (user_stack_allocator.used[i] == false) {
            stk_ptr = &user_stack_allocator.stk_pages[i];
            stk_ptr = (void*)((uint64_t)stk_ptr + KERNEL_STACK_SIZE);
            task_struct_allocator.used[i] = true;
            break;
        }
    }

    return stk_ptr;
}
