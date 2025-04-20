#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <sync.h>
#include <xkernel/sigset.h>

struct sigpending {
    sigset_t signal;
};

struct k_sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

typedef struct {
    struct lock lock;
    struct k_sigaction action[SIGMAX];
} sighand_t;


typedef struct {
	unsigned long csr_era;
	unsigned long csr_prmd;
	unsigned long __space[32];
} mcontext_t;

typedef struct __ucontext
{
	unsigned long      uc_stack;
	sigset_t           uc_sigmask;
	mcontext_t         uc_mcontext;
} ucontext_t;

#endif