#ifndef _ASM_PROCESSOR_H
#define _ASM_PROCESSOR_H

struct cpu_context {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
};

struct thread_struct {
	struct cpu_context	cpu_context;
};

#endif /* _ASM_PROCESSOR_H */
