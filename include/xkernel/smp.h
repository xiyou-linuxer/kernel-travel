#ifndef _LINUX_SMP_H
#define _LINUX_SMP_H

#include <asm/smp.h>

#ifndef __smp_processor_id
#define __smp_processor_id(x) raw_smp_processor_id(x)
#endif

#define smp_processor_id() __smp_processor_id()

#endif /* _LINUX_SMP_H */
