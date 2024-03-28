#ifndef _LINUX_IRQFLAGS_H
#define _LINUX_IRQFLAGS_H

#include <asm/irqflags.h>

#define raw_local_irq_disable()		arch_local_irq_disable()
#define raw_local_irq_enable()		arch_local_irq_enable()

#define local_irq_enable()	do { raw_local_irq_enable(); } while (0)
#define local_irq_disable()	do { raw_local_irq_disable(); } while (0)

#endif /* _LINUX_IRQFLAGS_H */
