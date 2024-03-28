#ifndef _LINUX_ATOMIC_INSTRUMENTED_H
#define _LINUX_ATOMIC_INSTRUMENTED_H

#include <asm/atomic.h>

static inline void
atomic_set(atomic_t *v, int i)
{
	arch_atomic_set(v, i);
}

#endif /* _LINUX_ATOMIC_INSTRUMENTED_H */
