#ifndef __ASM_ASM_H
#define __ASM_ASM_H

#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

#endif /* __ASM_ASM_H */
