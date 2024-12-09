#ifndef __ASM_GENERIC_BITOPS_ATOMIC_H
#define __ASM_GENERIC_BITOPS_ATOMIC_H

#include <xkernel/atomic.h>
#include <xkernel/compiler.h>
#include <xkernel/atomic/atomic-long.h>

static inline void set_bit(unsigned int nr, volatile unsigned long *p)
{
	p += BIT_WORD(nr);
	atomic_long_or(BIT_MASK(nr), (atomic_long_t *)p);
}

static inline void clear_bit(unsigned int nr, volatile unsigned long *p)
{
	p += BIT_WORD(nr);
	atomic_long_andnot(BIT_MASK(nr), (atomic_long_t *)p);
}

#endif /* __ASM_GENERIC_BITOPS_ATOMIC_H */
