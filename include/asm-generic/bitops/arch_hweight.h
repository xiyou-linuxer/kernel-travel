/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BITOPS_ARCH_HWEIGHT_H
#define __ASM_GENERIC_BITOPS_ARCH_HWEIGHT_H

#include <asm/types.h>

static inline unsigned int __arch_hweight32(unsigned int w)
{
	return __sw_hweight32(w);
}

static inline unsigned int __arch_hweight16(unsigned int w)
{
	return __sw_hweight16(w);
}

static inline unsigned int __arch_hweight8(unsigned int w)
{
	return __sw_hweight8(w);
}

static inline unsigned long __arch_hweight64(__u64 w)
{
	return __sw_hweight64(w);
}
#endif /* __ASM_GENERIC_BITOPS_HWEIGHT_H */
