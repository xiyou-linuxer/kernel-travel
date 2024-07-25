#ifndef __XKERNEL_ATOMIC_H
#define __XKERNEL_ATOMIC_H

#include <xkernel/bits.h>
#include <xkernel/compiler.h>
#include <asm/atomic.h>
#include <asm/barrier.h>

/**
 * atomic_add_unless - 如果起初原子类型v的值即与u相等，则返回0，否则返回非0值
 * @v: 原子变量指针
 * @a: 在v的值不为u时，原子变量的值将在其原有基础上增加该变量值
 * @u: 被用来与v的值进行比较来决定是否对v进行增加a
 *
 * 函数会原子地将a加到v指向的值上，
 * 但这种加法操作只有在v的当前值不等于u时才会执行
 */
static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
	return __atomic_add_unless(v, a, u) != u;
}

#endif /* __XKERNEL_ATOMIC_H */
