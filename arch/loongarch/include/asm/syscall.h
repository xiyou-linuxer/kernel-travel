#ifndef _SYSCALL_H
#define _SYSCALL_H
#include <linux/thread.h>
#include <asm/pt_regs.h>

#define NR_SYSCALLS 200
#define ENOSYS		38

static inline long __syscall0(long n)
{
	register long nr asm("a7") = n;
	register long retval asm("a0");
	asm volatile(
	"syscall 0"
	: "+r"(retval)
	: "r"(nr)
	: "$t0","$t1","$t2","$t3","$t4","$t5","$t6",
	  "$t7","$t8","memory");
	return retval;
}


static inline long __syscall1(long n,long ag0)
{
	register long nr asm("a7") = n;
	register long arg0 asm("a0") = ag0;
	register long retval asm("a0");
	asm volatile(
	"syscall 0"
	: "+r"(retval)
	: "r"(nr),"r"(arg0)
	: "$t0","$t1","$t2","$t3","$t4","$t5","$t6",
	  "$t7","$t8","memory");
	return retval;
}

static inline long __syscall2(long n,long ag0,long ag1)
{
	register long nr asm("a7") = n;
	register long arg0 asm("a0") = ag0;
	register long arg1 asm("a1") = ag1;
	register long retval asm("a0");
	asm volatile(
	"syscall 0"
	: "+r"(retval)
	: "r"(nr),"r"(arg0),"r"(arg1)
	: "$t0","$t1","$t2","$t3","$t4","$t5","$t6",
	  "$t7","$t8","memory");
	return retval;
}


static inline long __syscall3(long n,long ag0,long ag1,long ag2)
{
	register long nr asm("a7") = n;
	register long arg0 asm("a0") = ag0;
	register long arg1 asm("a1") = ag1;
	register long arg2 asm("a2") = ag2;
	register long retval asm("a0");
	asm volatile(
	"syscall 0"
	: "+r"(retval)
	: "r"(nr),"r"(arg0),"r"(arg1),"r"(arg2)
	: "$t0","$t1","$t2","$t3","$t4","$t5","$t6",
	  "$t7","$t8","memory");
	return retval;
}

static inline long __syscall4(long n,long ag0,long ag1,long ag2,long ag3)
{
	register long nr asm("a7") = n;
	register long arg0 asm("a0") = ag0;
	register long arg1 asm("a1") = ag1;
	register long arg2 asm("a2") = ag2;
	register long arg3 asm("a3") = ag3;
	register long retval asm("a0");
	asm volatile(
	"syscall 0"
	: "+r"(retval)
	: "r"(nr),"r"(arg0),"r"(arg1),"r"(arg2),"r"(arg3)
	: "$t0","$t1","$t2","$t3","$t4","$t5","$t6",
	  "$t7","$t8","memory");
	return retval;
}

#define _tl(x) ((long)x)
#define _syscall0(n) __syscall0(_tl(n))
#define _syscall1(n,a0) __syscall1(_tl(n),_tl(a0))
#define _syscall2(n,a0,a1) __syscall2(_tl(n),_tl(a0),_tl(a1))
#define _syscall3(n,a0,a1,a2) __syscall3(_tl(n),_tl(a0),_tl(a1),_tl(a2))
#define _syscall4(n,a0,a1,a2,a3) __syscall4(_tl(n),_tl(a0),_tl(a1),_tl(a2),_tl(a3))
#define _syscall5(n,a0,a1,a2,a3,a4) __syscall5(_tl(n),_tl(a0),_tl(a1),_tl(a2),_tl(a3),_tl(a4))

#define COUNT_ARGS_N(_n,_1,_2,_3,_4,_5,_6,num,...) num
#define COUNT_ARGS(...) COUNT_ARGS_N(__VA_ARGS__,6,5,4,3,2,1,0)

#define CONCAT_ARGS_REAL(a,b) a##b
#define CONCAT_ARGS(a,b) CONCAT_ARGS_REAL(a,b)
#define syscall(...)  CONCAT_ARGS(_syscall,COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

extern void* syscall_table[NR_SYSCALLS];

enum SYSCALL {
	SYS_PSTR,
	SYS_FORK,
};

#define SYS_write          64
#define SYS_gettimeofday  169
#define SYS_getpid        172
#define SYS_nanosleep     101

void __attribute__((__noinline__)) do_syscall(struct pt_regs *regs);

static inline int64_t write(int fd,const void* buf,size_t count) {
	return syscall(SYS_write,fd,buf,count);
}

static inline pid_t getpid(void) {
	return syscall(SYS_getpid);
}

static inline int gettimeofday(struct timespec *ts) {
	return syscall(SYS_gettimeofday,ts);
}

static inline int sleep(struct timespec *req,struct timespec *rem) {
	return syscall(SYS_nanosleep,req,rem);
}

static inline void pstr(char *str) {
	syscall(SYS_PSTR,str);
}

static inline int fork(void) {
	return syscall(SYS_FORK);
}


#endif
