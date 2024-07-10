#ifndef __ASM_GENERIC_BARRIER_H
#define __ASM_GENERIC_BARRIER_H

#ifndef __ASSEMBLY__

#include <xkernel/compiler.h>

#ifndef nop
#define nop()	asm volatile ("nop")
#endif

/*
 * Architectures that want generic instrumentation can define __ prefixed
 * variants of all barriers.
 */

#ifdef __mb
#define mb()	do { __mb(); } while (0)
#endif

#ifdef __rmb
#define rmb()	do { __rmb(); } while (0)
#endif

#ifdef __wmb
#define wmb()	do { __wmb(); } while (0)
#endif

#ifdef __dma_mb
#define dma_mb()	do { __dma_mb(); } while (0)
#endif

#ifdef __dma_rmb
#define dma_rmb()	do { __dma_rmb(); } while (0)
#endif

#ifdef __dma_wmb
#define dma_wmb()	do { __dma_wmb(); } while (0)
#endif

/*
 * Force strict CPU ordering. And yes, this is required on UP too when we're
 * talking to devices.
 *
 * Fall back to compiler barriers if nothing better is provided.
 */

#ifndef mb
#define mb()	barrier()
#endif

#ifndef rmb
#define rmb()	mb()
#endif

#ifndef wmb
#define wmb()	mb()
#endif

#ifndef dma_mb
#define dma_mb()	mb()
#endif

#ifndef dma_rmb
#define dma_rmb()	rmb()
#endif

#ifndef dma_wmb
#define dma_wmb()	wmb()
#endif

#ifndef __smp_mb
#define __smp_mb()	mb()
#endif

#ifndef __smp_rmb
#define __smp_rmb()	rmb()
#endif

#ifndef __smp_wmb
#define __smp_wmb()	wmb()
#endif

#ifdef CONFIG_SMP

#ifndef smp_mb
#define smp_mb()	do { __smp_mb(); } while (0)
#endif

#ifndef smp_rmb
#define smp_rmb()	do {__smp_rmb(); } while (0)
#endif

#ifndef smp_wmb
#define smp_wmb()	do { __smp_wmb(); } while (0)
#endif

#else	/* !CONFIG_SMP */

#ifndef smp_mb
#define smp_mb()	barrier()
#endif

#ifndef smp_rmb
#define smp_rmb()	barrier()
#endif

#ifndef smp_wmb
#define smp_wmb()	barrier()
#endif

#endif	/* CONFIG_SMP */

#ifndef __smp_store_mb
#define __smp_store_mb(var, value)  do { WRITE_ONCE(var, value); __smp_mb(); } while (0)
#endif

#ifndef __smp_mb__before_atomic
#define __smp_mb__before_atomic()	__smp_mb()
#endif

#ifndef __smp_mb__after_atomic
#define __smp_mb__after_atomic()	__smp_mb()
#endif

#ifndef __smp_store_release
#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	__smp_mb();							\
	WRITE_ONCE(*p, v);						\
} while (0)
#endif

#ifndef __smp_load_acquire
#define __smp_load_acquire(p)						\
({									\
	__unqual_scalar_typeof(*p) ___p1 = READ_ONCE(*p);		\
	compiletime_assert_atomic_type(*p);				\
	__smp_mb();							\
	(typeof(*p))___p1;						\
})
#endif

#ifdef CONFIG_SMP

#ifndef smp_store_mb
#define smp_store_mb(var, value)  do { __smp_store_mb(var, value); } while (0)
#endif

#ifndef smp_mb__before_atomic
#define smp_mb__before_atomic()	do { __smp_mb__before_atomic(); } while (0)
#endif

#ifndef smp_mb__after_atomic
#define smp_mb__after_atomic()	do { __smp_mb__after_atomic(); } while (0)
#endif

#ifndef smp_store_release
#define smp_store_release(p, v) do { __smp_store_release(p, v); } while (0)
#endif

#ifndef smp_load_acquire
#define smp_load_acquire(p) __smp_load_acquire(p)
#endif

#else	/* !CONFIG_SMP */

#ifndef smp_store_mb
#define smp_store_mb(var, value)  do { WRITE_ONCE(var, value); barrier(); } while (0)
#endif

#ifndef smp_mb__before_atomic
#define smp_mb__before_atomic()	barrier()
#endif

#ifndef smp_mb__after_atomic
#define smp_mb__after_atomic()	barrier()
#endif

#ifndef smp_store_release
#define smp_store_release(p, v)						\
do {									\
	barrier();							\
	WRITE_ONCE(*p, v);						\
} while (0)
#endif

#ifndef smp_load_acquire
#define smp_load_acquire(p)						\
({									\
	__unqual_scalar_typeof(*p) ___p1 = READ_ONCE(*p);		\
	barrier();							\
	(typeof(*p))___p1;						\
})
#endif

#endif	/* CONFIG_SMP */

/* Barriers for virtual machine guests when talking to an SMP host */
#define virt_mb() do { __smp_mb(); } while (0)
#define virt_rmb() do { __smp_rmb(); } while (0)
#define virt_wmb() do { __smp_wmb(); } while (0)
#define virt_store_mb(var, value) do { __smp_store_mb(var, value); } while (0)
#define virt_mb__before_atomic() do { __smp_mb__before_atomic(); } while (0)
#define virt_mb__after_atomic()	do { __smp_mb__after_atomic(); } while (0)
#define virt_store_release(p, v) do { __smp_store_release(p, v); } while (0)
#define virt_load_acquire(p) __smp_load_acquire(p)

/**
 * smp_acquire__after_ctrl_dep() - Provide ACQUIRE ordering after a control dependency
 *
 * A control dependency provides a LOAD->STORE order, the additional RMB
 * provides LOAD->LOAD order, together they provide LOAD->{LOAD,STORE} order,
 * aka. (load)-ACQUIRE.
 *
 * Architectures that do not do load speculation can have this be barrier().
 */
#ifndef smp_acquire__after_ctrl_dep
#define smp_acquire__after_ctrl_dep()		smp_rmb()
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ASM_GENERIC_BARRIER_H */
