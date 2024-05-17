/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_INIT_H
#define _LINUX_INIT_H

#include <xkernel/compiler_attributes.h>

#define __init		__section(".init.text") __cold
#define __initdata	__section(".init.data")
#define __initconst	__section(".init.rodata")
#define __exitdata	__section(".exit.data")
#define __exit_call	__used __section(".exitcall.exit")

/* For assembly routines */
#define __HEAD		.section	".head.text","ax"
#define __INIT		.section	".init.text","ax"
#define __FINIT		.previous

/* silence warnings when references are OK */
#define __REF		.section       ".ref.text", "ax"

/* Used for memory*/
#define __meminit	__section(".meminit.text")
#define __meminitdata	__section(".meminit.data")
#endif /* _LINUX_INIT_H */
