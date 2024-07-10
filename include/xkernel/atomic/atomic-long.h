#ifndef __XKERNEL_ATOMIC_LONG_H
#define __XKERNEL_ATOMIC_LONG_H

#include <xkernel/compiler.h>
#include <asm/types.h>

#ifdef CONFIG_64BIT
typedef atomic64_t atomic_long_t;
#define ATOMIC_LONG_INIT(i)		ATOMIC64_INIT(i)
#define atomic_long_cond_read_acquire	atomic64_cond_read_acquire
#define atomic_long_cond_read_relaxed	atomic64_cond_read_relaxed
#else
typedef atomic_t atomic_long_t;
#define ATOMIC_LONG_INIT(i)		ATOMIC_INIT(i)
#define atomic_long_cond_read_acquire	atomic_cond_read_acquire
#define atomic_long_cond_read_relaxed	atomic_cond_read_relaxed
#endif

#endif /* __XKERNEL_ATOMIC_LONG_H */
