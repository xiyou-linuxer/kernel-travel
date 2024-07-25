#ifndef __ASM_BITOPS_H
#define __ASM_BITOPS_H

#include <xkernel/compiler.h>

#ifndef __XKERNEL_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <asm/barrier.h>

#include <asm-generic/bitops/builtin-ffs.h>
#include <asm-generic/bitops/builtin-fls.h>
#include <asm-generic/bitops/builtin-__ffs.h>
#include <asm-generic/bitops/builtin-__fls.h>

#include <asm-generic/bitops/ffz.h>
#include <asm-generic/bitops/fls64.h>

#include <asm-generic/bitops/sched.h>
#include <asm-generic/bitops/hweight.h>

#include <asm-generic/bitops/non-atomic.h>

#endif /* __ASM_BITOPS_H */
