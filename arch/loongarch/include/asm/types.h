/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_TYPES_H
#define __ASM_TYPES_H

#include <asm-generic/types.h>

#ifdef __ASSEMBLY__
#define _ULCAST_
#define _U64CAST_
#else
#define _ULCAST_ (unsigned long)
#define _U64CAST_ (u64)
#endif

#endif /* __ASM_TYPES_H */
