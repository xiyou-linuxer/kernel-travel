#ifndef _LINUX_ATOMIC_INSTRUMENTED_H
#define _LINUX_ATOMIC_INSTRUMENTED_H

#ifdef CONFIG_LOONGARCH
#include <asm/atomic.h>
#endif

static inline void
atomic_set(atomic_t *v, int i)
{
#ifdef CONFIG_LOONGARCH
	arch_atomic_set(v, i);
#endif
}

#endif /* _LINUX_ATOMIC_INSTRUMENTED_H */
