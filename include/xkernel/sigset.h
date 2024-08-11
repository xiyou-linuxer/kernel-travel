#ifndef __SIGSET_H
#define __SIGSET_H

#include <debug.h>

#define SIGMAX 32
#define NSIG_WORDS ((SIGMAX+7)>>3)

typedef struct {
    unsigned long sig[NSIG_WORDS];
} sigset_t;

static inline void init_sigset(sigset_t *set)
{
	for (int i = 0; i < NSIG_WORDS; i++)
		set->sig[i] = 0;
}

static inline int sigismember(const sigset_t *set,int signo)
{
	ASSERT(0 < signo && signo <= SIGMAX);
	signo -= 1;
	return (1 << signo%8) & (set->sig[signo/8]);
}

static inline void sigaddset(sigset_t *set,int signo)
{
	ASSERT(0 < signo && signo <= SIGMAX);
	signo -= 1;
	set->sig[signo/8] |= (1 << signo%8);
}

static inline void sigremove(sigset_t *set,int signo)
{
	ASSERT(0 < signo && signo <= SIGMAX);
	signo -= 1;
	set->sig[signo/8] &= ~(1 << signo%8);
}

static inline void sigset_or(sigset_t *set1, sigset_t *set2) {
	for (int i = 0; i < NSIG_WORDS; i++) {
		set1->sig[i] |= set2->sig[i];
	}
}

#endif
