#ifndef _ASM_ADDRSPACE_H
#define _ASM_ADDRSPACE_H

#include <asm/loongarch.h>

#define DMW_PABITS	48
#define TO_PHYS_MASK	((1ULL << DMW_PABITS) - 1)

#ifdef __ASSEMBLY__
#define _ATYPE_
#define _ATYPE32_
#define _ATYPE64_
#define _CONST64_(x)	x
#else
#define _ATYPE_		__PTRDIFF_TYPE__
#define _ATYPE32_	int
#define _ATYPE64_	__s64
#define _CONST64_(x)	x ## L
#endif

/*
 *  32/64-bit LoongArch address spaces
 */
#ifdef __ASSEMBLY__
#define _ACAST32_
#define _ACAST64_
#else
#define _ACAST32_		(_ATYPE_)(_ATYPE32_)	/* widen if necessary */
#define _ACAST64_		(_ATYPE64_)		/* do _not_ narrow */
#endif
#define CACHE_BASE		CSR_DMW1_BASE
#define PHYSADDR(a)		((_ACAST64_(a)) & TO_PHYS_MASK)
#define TO_CACHE(x)		(CACHE_BASE   |	((x) & TO_PHYS_MASK))//取出后48位偏移地址并与cache的基地址相或取得虚拟地址（前文中将虚拟地址映射到了cache之中
#endif /* _ASM_ADDRSPACE_H */
