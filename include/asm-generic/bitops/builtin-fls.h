/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BITOPS_BUILTIN_FLS_H
#define __ASM_GENERIC_BITOPS_BUILTIN_FLS_H

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static __always_inline int fls(unsigned int x)
{
	return x ? sizeof(x) * 8 - __builtin_clz(x) : 0;
}

#endif /* __ASM_GENERIC_BITOPS_BUILTIN_FLS_H */
