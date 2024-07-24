#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <sync.h>
#include <xkernel/sched.h>
#include <xkernel/sigset.h>

#define SIGHUP           1
#define SIGINT           2
#define SIGQUIT          3
#define SIGILL           4
#define SIGTRAP          5
#define SIGABRT          6
#define SIGIOT           SIGABRT
#define SIGBUS           7
#define SIGFPE           8
#define SIGKILL          9
#define SIGUSR1         10
#define SIGSEGV         11
#define SIGUSR2         12
#define SIGPIPE         13
#define SIGALRM         14
#define SIGTERM         15
#define SIGSTKFLT       16
#define SIGCHLD         17
#define SIGCONT         18
#define SIGSTOP         19
#define SIGTSTP         20
#define SIGTTIN         21
#define SIGTTOU         22
#define SIGURG          23
#define SIGXCPU         24
#define SIGXFSZ         25
#define SIGVTALRM       26
#define SIGPROF         27
#define SIGWINCH        28
#define SIGIO           29
#define SIGPOLL         SIGIO
#define SIGPWR          30
#define SIGSYS          31
#define SIGUNUSED       SIGSYS
#define _NSIG           65


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

int group_sendsig(); //future work
int specific_sendsig(int sig,struct task_struct* t);


#endif
