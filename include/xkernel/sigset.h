#ifndef __SIGSET_H
#define __SIGSET_H

#include <debug.h>

#define SIGMAX 32

typedef struct {
    unsigned long sig[(SIGMAX+7) & ~7];
} sigset_t;

static inline int sigismember(const sigset_t *set,int signo)
{
	ASSERT(0 < signo && signo <= SIGMAX);
	return (1 << signo%8) & (set->sig[signo/8]);
}


#endif
