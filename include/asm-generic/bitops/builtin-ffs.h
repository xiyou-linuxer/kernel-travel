#ifndef __ASM_GENERIC_BITOPS_BUILTIN_FFS_H
#define __ASM_GENERIC_BITOPS_BUILTIN_FFS_H

/**
 * ffs - find first bit set
 * @x: the word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from ffz (man ffs).
 */
#define ffs(x) __builtin_ffs(x)

#endif /* __ASM_GENERIC_BITOPS_BUILTIN_FFS_H */
