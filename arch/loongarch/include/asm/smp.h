#ifndef _ASM_SMP_H
#define _ASM_SMP_H

#include <asm/thread_info.h>

static inline int raw_smp_processor_id(void)
{
	return current_thread_info()->cpu;
}
#define raw_smp_processor_id raw_smp_processor_id

#endif /* _ASM_SMP_H */
