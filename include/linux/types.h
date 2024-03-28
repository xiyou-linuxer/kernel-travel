#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#ifndef __ASSEMBLY__

#include <asm-generic/bitsperlong.h>

#undef offsetof
#define offsetof(t,m) ((size_t)&((t *)0)->m)

#define asmlinkage __attribute__((regparm(0)))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define NULL ((void*)0)

#define true 1
#define false 0

typedef enum {
    FALSE = 0,
    TRUE = 1
} bool;

typedef unsigned char		u8;
typedef u8			        __u8;
typedef unsigned short		u16;
typedef u16                 __u16;
typedef unsigned int		u32;
typedef u32                 __u32;
typedef unsigned long long	u64;
typedef u64			        __u64;

typedef signed short		s16;
typedef signed long long    __s64;

typedef u8                  uint8_t;
typedef u16                 uint16_t;
typedef u32                 uint32_t;
typedef u64                 uint64_t;

typedef unsigned long		size_t;
typedef u64			        phys_addr_t;

#define __aligned_u64 __u64 __attribute__((aligned(8)))

typedef struct {
	__u8 b[16];
} guid_t;

#define INT_MAX			((int)(~0U>>1))
#define UINT32_MAX		((u32)~0U)
#define INT32_MAX		((s32)(UINT32_MAX >> 1))

typedef struct {
	int counter;
} atomic_t;

#define _ULCAST_ (unsigned long)
#define _U64CAST_ (u64)

#ifdef __CHECKER__
#define __bitwise	__attribute__((bitwise))
#else
#define __bitwise
#endif

/* The kernel doesn't use this legacy form, but user space does */
#define __bitwise__ __bitwise

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

#define __aligned_u64 __u64 __attribute__((aligned(8)))
#define __aligned_be64 __be64 __attribute__((aligned(8)))
#define __aligned_le64 __le64 __attribute__((aligned(8)))

typedef unsigned __bitwise __poll_t;

#endif /* !__ASSEMBLY__ */

#endif /* _LINUX_TYPES_ */
