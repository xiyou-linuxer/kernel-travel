#ifndef _SYSCALL_H
#define _SYSCALL_H
#include <linux/thread.h>
#include <asm/pt_regs.h>

#define NR_SYSCALLS 48
#define ENOSYS		38

#define __syscall0(n) ({    \
		register long nr asm("a7") = n; \
		register long retval asm("a0"); \
        asm volatile(       \
        "syscall 0"         \
        : "=r"(retval)     \
        : "r"(n)           \
        : "$t0","$t1","$t2","$t3","$t4","$t5","$t6", \
		  "$t7","$t8","memory");                  \
        retval;             \
    })

asmlinkage static inline long __syscall1(long n,long ag0)
{
	register long nr asm("a7") = n;
	register long arg0 asm("a0") = ag0;
	//register long retval asm("a0");
	asm volatile(
	"syscall 0"
	: //"+r"(retval)
	: "r"(nr),"r"(arg0)
	: "$t0","$t1","$t2","$t3","$t4","$t5","$t6",
	  "$t7","$t8","memory");
	//return retval;
	return 1;
}

#define __syscall2(n,ag0,ag1) ({    \
		register long nr asm("a7") = n; \
		register long retval asm("a0"); \
		register long arg0 asm("a0"); \
		register long arg1 asm("a1"); \
        asm volatile(             \
        "syscall 0"               \
        : "+r"(retval)           \
        : "r"(n),"r"(arg0),"r"(arg1)        \
        : "$t0","$t1","$t2","$t3","$t4","$t5","$t6", \
		  "$t7","$t8","memory");                  \
        retval;                   \
    })

#define __syscall3(n,a0,a1,a2) ({    \
		register long nr asm("a7") = n; \
		register long retval asm("a0"); \
		register long arg0 asm("a0"); \
		register long arg1 asm("a1"); \
		register long arg2 asm("a2"); \
        : "+r"(retval)              \
        : "r"(n),"r"(arg0),"r"(arg1),"r"(arg2)  \
        : "$t0","$t1","$t2","$t3","$t4","$t5","$t6", \
		  "$t7","$t8","memory");                  \
        retval;                   \
    })

#define __syscall4(n,a0,a1,a2,a3) ({            \
		register long nr asm("a7") = n; \
		register long retval asm("a0"); \
		register long arg0 asm("a0"); \
		register long arg1 asm("a1"); \
		register long arg2 asm("a2"); \
		register long arg3 asm("a3"); \
        asm volatile(                        \
        "syscall 0"                          \
        : "+r"(retval)                      \
        : "r"(n),"r"(arg0),"r"(arg1),"r"(arg2),"r"(arg3) \
        : "$t0","$t1","$t2","$t3","$t4","$t5","$t6", \
		  "$t7","$t8","memory");                  \
        retval;                              \
    })


#define _tl(x) ((long)x)
#define _syscall0(n) __syscall0(_tl(n))
#define _syscall1(n,a0) __syscall1(_tl(n),_tl(a0))
#define _syscall2(n,a0,a1) __syscall2(_tl(n),_tl(a0),tl(a1))
#define _syscall3(n,a0,a1,a2) __syscall3(_tl(n),_tl(a0),tl(a1),tl(a2))
#define _syscall4(n,a0,a1,a2,a3) __syscall4(_tl(n),_tl(a0),tl(a1),tl(a2),tl(a3))
#define _syscall5(n,a0,a1,a2,a3,a4) __syscall5(_tl(n),_tl(a0),tl(a1),tl(a2),tl(a3),tl(a4))

#define COUNT_ARGS_N(_n,_1,_2,_3,_4,_5,_6,num,...) num
#define COUNT_ARGS(...) COUNT_ARGS_N(__VA_ARGS__,6,5,4,3,2,1,0)

#define CONCAT_ARGS_REAL(a,b) a##b
#define CONCAT_ARGS(a,b) CONCAT_ARGS_REAL(a,b)
#define syscall(...)  CONCAT_ARGS(_syscall,COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

extern void* syscall_table[NR_SYSCALLS];

enum SYSCALL {
	SYS_GETPID,
	SYS_PSTR,
};


void __attribute__((__noinline__)) do_syscall(struct pt_regs *regs);

pid_t getpid(void);
void pstr(char *str);


#endif
