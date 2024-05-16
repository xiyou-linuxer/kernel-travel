#ifndef _ASM_CACHE_H
#define _ASM_CACHE_H

#include <xkernel/linkage.h>

#define CONFIG_L1_CACHE_SHIFT	6
#define L1_CACHE_SHIFT	CONFIG_L1_CACHE_SHIFT
#define L1_CACHE_BYTES	(1 << L1_CACHE_SHIFT)

#define __read_mostly __section(".data..read_mostly")

#endif /* _ASN_CACHE_H */
