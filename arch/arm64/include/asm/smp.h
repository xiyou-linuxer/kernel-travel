#ifndef __ASM_SMP_H
#define __ASM_SMP_H

#define raw_smp_processor_id() (current_thread_info()->cpu)

#endif /* __ASM_SMP_H */
