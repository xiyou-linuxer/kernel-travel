#include <allocator.h>
#include <xkernel/memory.h>


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
    task = (struct task_struct*)get_page();
    return task;
}


void* userstk_alloc(uint64_t pdir)
{
	void *stk_ptr = NULL;
	malloc_usrpage(pdir,USER_STACK-KERNEL_STACK_SIZE);
	stk_ptr = (void*)(USER_STACK);

	return stk_ptr;
}
