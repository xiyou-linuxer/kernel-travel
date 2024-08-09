#include <signal.h>
#include <xkernel/thread.h>
#include <asm-generic/bitops/ffz.h>
#include <xkernel/stdio.h>
#include <xkernel/sched.h>

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

#define M(sig) (1ULL << ((sig)-1))
#define T(sig, mask) (M(sig) & (mask))

#define SIG_KERNEL_IGNORE_MASK (\
		M(SIGCONT) | M(SIGCHLD) | M(SIGWINCH) | M(SIGURG) )

#define sig_kernel_ignore(sig) T(sig, SIG_KERNEL_IGNORE_MASK)

static inline int find_lsb(unsigned long *x) {
	for (int i = 0; i < sizeof(unsigned long)*8; i++) {
		if (*x & (1<<i))
			return i;
	}
	return 0;
}


void init_handlers(sighand_t **handlers)
{
	sighand_t *p = *handlers = (sighand_t*)get_page();
	lock_init(&p->lock);
	memset(p->action,0,sizeof(p->action));
}

bool isignored_signals(int sig,struct task_struct *t)
{
	struct sigpending *pending = &t->pending;

	/* 被屏蔽的信号不能被忽略,解除屏蔽后信号处理可能改变 */
	if (sigismember(&t->blocked,sig))
		return 0;

	/* 两种情况：
	 * 一、信号处理为忽略
	 * 二、默认，且该信号的默认行为是忽略 */
	sighand_t *handlers = t->handlers;
	struct k_sigaction ac = handlers->action[sig-1];
	return  (ac.sa_handler == SIG_IGN) ||
			(ac.sa_handler == SIG_DFL && sig_kernel_ignore(sig));
}

int specific_sendsig(int sig,struct task_struct *t)
{
	/* 如果信号是被忽略的，则不产生 */
	if (isignored_signals(sig,t))
		return 0;
	sigaddset(&t->pending.signal,sig);

	return sig;
}

int get_signal(struct sigpending *pending,sigset_t *blocked)
{
	unsigned long *s = pending->signal.sig;
	unsigned long *m = blocked->sig;

	for (int i = 0; i < NSIG_WORDS; i++,s++,m++)
		if (*s &&~ *m)
			return i*8+find_lsb(s)+1;
	return 0;
}

int dequeue_signal(struct sigpending *pending,sigset_t *blocked)
{
	int sig = get_signal(pending,blocked);
	if (sig != 0)
		sigremove(&pending->signal,sig);
	return sig;
}

void check_signals(void)
{
	struct task_struct *cur = running_thread();

	int sig = dequeue_signal(&cur->pending,&cur->blocked);
	while (sig)
	{
		printk("receive sig:%d\n",sig);

		sig = dequeue_signal(&cur->pending,&cur->blocked);
	}
}

int sys_sigaction(int sig, const struct k_sigaction *act, struct k_sigaction *oldact)
{
	struct task_struct *cur = running_thread();
	specific_sendsig(sig,cur);
	return 0;
}


