#ifndef __ASM_PROCESS_H
#define __ASM_PROCESS_H

#ifdef __KERNEL__

#ifndef __ASSEMBLY__

#include <asm/types.h>

/**
 * low level task data that entry.S needs immediate access to.
 */
struct thread_info {
        unsigned long           flags;
        struct task_struct      *task;
        int                     preempt_count;
        int                     cpu;
};

#endif /* __ASSEMBLY__ */

#endif /* __KERNEL__ */

#endif /* __ASM_PROCESS_H */
