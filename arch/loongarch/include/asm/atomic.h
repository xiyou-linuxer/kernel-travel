#ifndef _ASM_ATOMIC_H
#define _ASM_ATOMIC_H

#include <linux/types.h>
#include <asm-generic/rwonce.h>
#include <asm/barrier.h>

#if __SIZEOF_LONG__ == 4
#define __LL		"ll.w	"
#define __SC		"sc.w	"
#define __AMADD		"amadd.w	"
#define __AMAND_DB	"amand_db.w	"
#define __AMOR_DB	"amor_db.w	"
#define __AMXOR_DB	"amxor_db.w	"
#elif __SIZEOF_LONG__ == 8
#define __LL		"ll.d	"
#define __SC		"sc.d	"
#define __AMADD		"amadd.d	"
#define __AMAND_DB	"amand_db.d	"
#define __AMOR_DB	"amor_db.d	"
#define __AMXOR_DB	"amxor_db.d	"
#endif

#define ATOMIC_INIT(i)	  { (i) }

/*
 * arch_atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define arch_atomic_read(v)	READ_ONCE((v)->counter)

/*
 * arch_atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
#define arch_atomic_set(v, i)	WRITE_ONCE((v)->counter, (i))

#define ATOMIC_OP(op, I, asm_op)					\
static inline void arch_atomic_##op(int i, atomic_t *v)			\
{									\
	__asm__ __volatile__(						\
	"am"#asm_op"_db.w" " $zero, %1, %0	\n"			\
	: "+ZB" (v->counter)						\
	: "r" (I)							\
	: "memory");							\
}

#define ATOMIC_OP_RETURN(op, I, asm_op, c_op)				\
static inline int arch_atomic_##op##_return_relaxed(int i, atomic_t *v)	\
{									\
	int result;							\
									\
	__asm__ __volatile__(						\
	"am"#asm_op"_db.w" " %1, %2, %0		\n"			\
	: "+ZB" (v->counter), "=&r" (result)				\
	: "r" (I)							\
	: "memory");							\
									\
	return result c_op I;						\
}

#define ATOMIC_FETCH_OP(op, I, asm_op)					\
static inline int arch_atomic_fetch_##op##_relaxed(int i, atomic_t *v)	\
{									\
	int result;							\
									\
	__asm__ __volatile__(						\
	"am"#asm_op"_db.w" " %1, %2, %0		\n"			\
	: "+ZB" (v->counter), "=&r" (result)				\
	: "r" (I)							\
	: "memory");							\
									\
	return result;							\
}

#define ATOMIC_OPS(op, I, asm_op, c_op)					\
	ATOMIC_OP(op, I, asm_op)					\
	ATOMIC_OP_RETURN(op, I, asm_op, c_op)				\
	ATOMIC_FETCH_OP(op, I, asm_op)

ATOMIC_OPS(add, i, add, +)
ATOMIC_OPS(sub, -i, add, +)

#define arch_atomic_add_return_relaxed	arch_atomic_add_return_relaxed
#define arch_atomic_sub_return_relaxed	arch_atomic_sub_return_relaxed
#define arch_atomic_fetch_add_relaxed	arch_atomic_fetch_add_relaxed
#define arch_atomic_fetch_sub_relaxed	arch_atomic_fetch_sub_relaxed

#undef ATOMIC_OPS

#define ATOMIC_OPS(op, I, asm_op)					\
	ATOMIC_OP(op, I, asm_op)					\
	ATOMIC_FETCH_OP(op, I, asm_op)

ATOMIC_OPS(and, i, and)
ATOMIC_OPS(or, i, or)
ATOMIC_OPS(xor, i, xor)

#define arch_atomic_fetch_and_relaxed	arch_atomic_fetch_and_relaxed
#define arch_atomic_fetch_or_relaxed	arch_atomic_fetch_or_relaxed
#define arch_atomic_fetch_xor_relaxed	arch_atomic_fetch_xor_relaxed

#undef ATOMIC_OPS
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN
#undef ATOMIC_OP

static inline int arch_atomic_fetch_add_unless(atomic_t *v, int a, int u)
{
       int prev, rc;

	__asm__ __volatile__ (
		"0:	ll.w	%[p],  %[c]\n"
		"	beq	%[p],  %[u], 1f\n"
		"	add.w	%[rc], %[p], %[a]\n"
		"	sc.w	%[rc], %[c]\n"
		"	beqz	%[rc], 0b\n"
		"	b	2f\n"
		"1:\n"
		__WEAK_LLSC_MB
		"2:\n"
		: [p]"=&r" (prev), [rc]"=&r" (rc),
		  [c]"=ZB" (v->counter)
		: [a]"r" (a), [u]"r" (u)
		: "memory");

	return prev;
}
#define arch_atomic_fetch_add_unless arch_atomic_fetch_add_unless

static inline int arch_atomic_sub_if_positive(int i, atomic_t *v)
{
	int result;
	int temp;

	if (__builtin_constant_p(i)) {
		__asm__ __volatile__(
		"1:	ll.w	%1, %2		# atomic_sub_if_positive\n"
		"	addi.w	%0, %1, %3				\n"
		"	move	%1, %0					\n"
		"	bltz	%0, 2f					\n"
		"	sc.w	%1, %2					\n"
		"	beqz	%1, 1b					\n"
		"2:							\n"
		__WEAK_LLSC_MB
		: "=&r" (result), "=&r" (temp), "+ZC" (v->counter)
		: "I" (-i));
	} else {
		__asm__ __volatile__(
		"1:	ll.w	%1, %2		# atomic_sub_if_positive\n"
		"	sub.w	%0, %1, %3				\n"
		"	move	%1, %0					\n"
		"	bltz	%0, 2f					\n"
		"	sc.w	%1, %2					\n"
		"	beqz	%1, 1b					\n"
		"2:							\n"
		__WEAK_LLSC_MB
		: "=&r" (result), "=&r" (temp), "+ZC" (v->counter)
		: "r" (i));
	}

	return result;
}

#define arch_atomic_cmpxchg(v, o, n) (arch_cmpxchg(&((v)->counter), (o), (n)))
#define arch_atomic_xchg(v, new) (arch_xchg(&((v)->counter), (new)))

/*
 * arch_atomic_dec_if_positive - decrement by 1 if old value positive
 * @v: pointer of type atomic_t
 */
#define arch_atomic_dec_if_positive(v)	arch_atomic_sub_if_positive(1, v)

#endif /* _ASM_ATOMIC_H */
