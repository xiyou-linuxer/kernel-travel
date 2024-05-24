#ifndef _SYSCALL_H
#define _SYSCALL_H
#include <xkernel/thread.h>
#include <asm/pt_regs.h>
#include <xkernel/stdio.h>
#define NR_SYSCALLS 300
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

static inline long __syscall6(long n,long ag0,long ag1,long ag2,long ag3,long ag4,long ag5)
{
	register long nr asm("a7") = n;
	register long arg0 asm("a0") = ag0;
	register long arg1 asm("a1") = ag1;
	register long arg2 asm("a2") = ag2;
	register long arg3 asm("a3") = ag3;
	register long arg4 asm("a4") = ag4;
	register long arg5 asm("a5") = ag5;
	register long retval asm("a0");
	asm volatile(
	"syscall 0"
	: "+r"(retval)
	: "r"(nr),"r"(arg0),"r"(arg1),"r"(arg2),"r"(arg3),"r"(arg4),"r"(arg5)
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
#define _syscall6(n,a0,a1,a2,a3,a4,a5) __syscall6(_tl(n),_tl(a0),_tl(a1),_tl(a2),_tl(a3),_tl(a4),_tl(a5))

#define COUNT_ARGS_N(_n,_1,_2,_3,_4,_5,_6,num,...) num
#define COUNT_ARGS(...) COUNT_ARGS_N(__VA_ARGS__,6,5,4,3,2,1,0)

#define CONCAT_ARGS_REAL(a,b) a##b
#define CONCAT_ARGS(a,b) CONCAT_ARGS_REAL(a,b)
#define syscall(...)  CONCAT_ARGS(_syscall,COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

extern void* syscall_table[NR_SYSCALLS];

#define SYS_PSTR 222

#define AT_FDCWD 0
#define AT_OPEN -100

#define open(filename, flags) openat(AT_OPEN, (filename), (flags), (066))
#define mkdir(path, mode) mkdirat(AT_FDCWD,(path),(mode))
#define unlink(path) unlinkat(AT_FDCWD,(path),0)

#define SYS_getcwd         17
#define SYS_dup            23
#define SYS_dup2           24
#define SYS_mkdirat        34
#define SYS_unlinkat       35
#define SYS_umount2        39
#define SYS_mount          40
#define SYS_chdir          49
#define SYS_openat         56
#define SYS_close          57
#define SYS_read           63
#define SYS_write          64
#define SYS_fstat          80
#define SYS_exit           93
#define SYS_nanosleep     101
#define SYS_gettimeofday  169
#define SYS_getpid        172
#define SYS_clone         220
#define SYS_execve        221
#define SYS_mmap          222
#define SYS_wait4         260

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

static inline void exit(int status) {
	syscall(SYS_exit,status);
}

static inline pid_t wait(int* status) {
	return syscall(SYS_wait4,-1,status,0);
}

static inline int execve(const char *path, char *const argv[], char *const envp[]) {
	return syscall(SYS_execve,path,argv,envp);
}

static inline void pstr(char *str) {
	syscall(SYS_PSTR,str);
}

static inline int fork(void) {
	return syscall(SYS_clone,0,0);
}

static inline void * mmap(void * addr,
	unsigned long len, unsigned long prot,
	unsigned long flag, int fd, unsigned long offset) {
	return syscall(SYS_mmap, addr, len, prot, flag, fd, offset);
}

static inline int fstat(int fd,struct kstat* stat)
{
	return syscall(SYS_fstat, fd, stat);
}

#endif
