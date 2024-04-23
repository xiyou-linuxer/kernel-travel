#include <sync.h>
#include <trap/irq.h>
#include <linux/thread.h>
#include <debug.h>


void sema_init(struct semaphore* sema,uint8_t value)
{
    sema->value = value;
    list_init(&sema->wait_list);
}

void sema_down(struct semaphore* sema)
{
    enum intr_status old_status = intr_disable();
    struct task_struct* cur = (struct task_struct*)running_thread();
    while (sema->value == 0){
        ASSERT(!elem_find(&sema->wait_list,&cur->general_tag));
        if (elem_find(&sema->wait_list,&cur->general_tag)) {
            BUG();
        }
        list_append(&sema->wait_list,&cur->general_tag);
        thread_block(TASK_BLOCKED);
    }
    sema->value--;
    intr_set_status(old_status);
}

void sema_up(struct semaphore* sema)
{
    enum intr_status old_status = intr_disable();
    if (!list_empty(&sema->wait_list))
    {
        struct list_elem* b_tag = list_pop(&sema->wait_list);
        struct task_struct* b_pcb = \
        elem2entry(struct task_struct,general_tag,b_tag);
        thread_unblock(b_pcb);
    }
    sema->value++;
    intr_set_status(old_status);
}

void lock_init(struct lock* lk)
{
    lk->holder = NULL;
    sema_init(&lk->sema,1);
    lk->acquire_nr = 0;
}

void lock_acquire(struct lock* lk)
{
    if (lk->holder != running_thread()) {
        sema_down(&lk->sema);
        lk->holder = running_thread();
        lk->acquire_nr = 1;
    } else {
        lk->acquire_nr++;
    }
}

void lock_release(struct lock* lk)
{
    ASSERT(lk->acquire_nr != 0);
    if (lk->holder != running_thread()){
        BUG();
    }
    if (lk->acquire_nr == 1) {
        lk->acquire_nr = 0;
        lk->holder = NULL;  //得放在sema_up前面
        sema_up(&lk->sema);
    } else {
        lk->acquire_nr--;
    }
}
