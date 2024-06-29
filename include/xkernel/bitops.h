#ifndef __XKERNEL_BITOPS_H
#define __XKERNEL_BITOPS_H

#include <asm/types.h>
#include <xkernel/bits.h>
#include <xkernel/typecheck.h>

#define BITS_PER_TYPE(type)	(sizeof(type) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)	__KERNEL_DIV_ROUND_UP(nr, BITS_PER_TYPE(long))
#define BITS_TO_U64(nr)		__KERNEL_DIV_ROUND_UP(nr, BITS_PER_TYPE(u64))
#define BITS_TO_U32(nr)		__KERNEL_DIV_ROUND_UP(nr, BITS_PER_TYPE(u32))
#define BITS_TO_BYTES(nr)	__KERNEL_DIV_ROUND_UP(nr, BITS_PER_TYPE(char))

#define BYTES_TO_BITS(nb)	((nb) * BITS_PER_BYTE)

extern unsigned int __sw_hweight8(unsigned int w);
extern unsigned int __sw_hweight16(unsigned int w);
extern unsigned int __sw_hweight32(unsigned int w);
extern unsigned long __sw_hweight64(__u64 w);

/*
 * Defined here because those may be needed by architecture-specific static
 * inlines.
 */

#include <asm-generic/bitops/generic-non-atomic.h>

#endif /* __XKERNEL_BITOPS_H */
