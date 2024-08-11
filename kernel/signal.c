#include <allocator.h>
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

#define SIGRETURN_ADDR USER_TOP-0x2000
extern void sigreturn_code(void);

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

int sys_kill(pid_t pid, int sig)
{
	printk("kill %d sig %d\n",pid,sig);
	struct task_struct *t = pid2thread(pid);
	return specific_sendsig(sig,t);
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

static void save_sigcontext(struct pt_regs *regs,struct sigframe *f,sigset_t *blocked)
{
	ucontext_t *uc = &f->uc;
	memcpy(&uc->uc_sigmask,blocked,sizeof(sigset_t));
	uc->uc_stack = regs->regs[3];

	uc->uc_mcontext.csr_era  = regs->csr_era;
	uc->uc_mcontext.csr_prmd = regs->csr_prmd; 
	for (int r = 0; r < 32; r++) {
		uc->uc_mcontext.__space[r] = regs->regs[r];
	}
}

static void restore_sigcontext(struct pt_regs *regs,struct sigframe *f)
{
	struct task_struct *cur = running_thread();
	ucontext_t *uc = &f->uc;
	memcpy(&cur->blocked,&uc->uc_sigmask,sizeof(sigset_t));
	regs->regs[3] = uc->uc_stack;

	regs->csr_era  = uc->uc_mcontext.csr_era;
	regs->csr_prmd = uc->uc_mcontext.csr_prmd; 
	for (int r = 0; r < 32; r++) {
		regs->regs[r] = uc->uc_mcontext.__space[r];
	}
}

void sys_sigreturn(struct pt_regs *regs)
{
	struct sigframe *f = (struct sigframe*)regs->regs[3];
	restore_sigcontext(regs,f);
}

static void set_sigframe(int sig,struct pt_regs* regs,sigset_t *blocked)
{
	/* 保存当前用户进程上下文 */
	struct sigframe *frame = (struct sigframe*)(regs->regs[3] - sizeof(struct sigframe));
	save_sigcontext(regs,frame,blocked);

	/* 修改regs,以正确的状态进入信号处理程序 */
	struct task_struct *cur = running_thread();
	memcpy((void*)SIGRETURN_ADDR,sigreturn_code,0x20);
	regs->regs[1] = SIGRETURN_ADDR;
	regs->regs[3] = (uint64_t)frame;
	regs->regs[4] = (uint64_t)sig;
	regs->csr_era = (uint64_t)cur->handlers->action[sig-1].sa_handler;
}

void check_signals(struct pt_regs* regs)
{
	struct task_struct *cur = running_thread();
	sigset_t *blocked = &cur->blocked;

	int sig = dequeue_signal(&cur->pending,&cur->blocked);
	while (sig)
	{
		printk("receive sig:%d\n",sig);

		/* SIGKILL 永不阻塞，直接杀死对方 */
		if (sig == SIGKILL) {

		}

		/* 若取得的信号被阻塞,则重新加入 */
		if (sigismember(blocked,sig)) {
			specific_sendsig(sig,cur);
			sig = dequeue_signal(&cur->pending,&cur->blocked);
			continue;
		}

		struct k_sigaction *ka = &cur->handlers->action[sig-1];
		if (ka->sa_handler != SIG_DFL) {
			sigset_or(&cur->blocked,&ka->sa_mask);
			set_sigframe(sig,regs,blocked);
			break;
		}

		/* do default */
		if (sig_kernel_ignore(sig)) {
			continue;
		}

		sig = dequeue_signal(&cur->pending,&cur->blocked);
	}
}

int sys_sigaction(int sig, const struct k_sigaction *act, struct k_sigaction *oldact)
{
	printk("sys_sigaction\n");
	struct task_struct *cur = running_thread();
	memcpy(&cur->handlers->action[sig-1],act,sizeof(*act));
	return 0;
}


