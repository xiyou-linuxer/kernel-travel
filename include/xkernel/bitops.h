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

/*
 * Many architecture-specific non-atomic bitops contain inline asm code and due
 * to that the compiler can't optimize them to compile-time expressions or
 * constants. In contrary, generic_*() helpers are defined in pure C and
 * compilers optimize them just well.
 * Therefore, to make `unsigned long foo = 0; __set_bit(BAR, &foo)` effectively
 * equal to `unsigned long foo = BIT(BAR)`, pick the generic C alternative when
 * the arguments can be resolved at compile time. That expression itself is a
 * constant and doesn't bring any functional changes to the rest of cases.
 * The casts to `uintptr_t` are needed to mitigate `-Waddress` warnings when
 * passing a bitmap from .bss or .data (-> `!!addr` is always true).
 */
#define bitop(op, nr, addr)						\
	((__builtin_constant_p(nr) &&					\
	  __builtin_constant_p((uintptr_t)(addr) != (uintptr_t)NULL) &&	\
	  (uintptr_t)(addr) != (uintptr_t)NULL &&			\
	  __builtin_constant_p(*(const unsigned long *)(addr))) ?	\
	 const##op(nr, addr) : op(nr, addr))

#define __set_bit(nr, addr)		bitop(___set_bit, nr, addr)
#define __clear_bit(nr, addr)		bitop(___clear_bit, nr, addr)
#define __change_bit(nr, addr)		bitop(___change_bit, nr, addr)
#define __test_and_set_bit(nr, addr)	bitop(___test_and_set_bit, nr, addr)
#define __test_and_clear_bit(nr, addr)	bitop(___test_and_clear_bit, nr, addr)
#define __test_and_change_bit(nr, addr)	bitop(___test_and_change_bit, nr, addr)
#define test_bit(nr, addr)		bitop(_test_bit, nr, addr)
#define test_bit_acquire(nr, addr)	bitop(_test_bit_acquire, nr, addr)

/*
 * Include this here because some architectures need generic_ffs/fls in
 * scope
 */
#include <asm/bitops.h>

#endif /* __XKERNEL_BITOPS_H */
