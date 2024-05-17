#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include <stdint.h>
#include <xkernel/list.h>

struct semaphore {
    uint8_t value;
    struct list wait_list;
};

struct lock {
    struct task_struct* holder;
    struct semaphore sema;
    uint32_t acquire_nr;
};

void sema_init(struct semaphore*,uint8_t);
void sema_down(struct semaphore*);
void sema_up(struct semaphore*);
void lock_init(struct lock*);
void lock_acquire(struct lock*);
void lock_release(struct lock*);

#endif
