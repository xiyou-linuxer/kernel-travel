#ifndef _SETUP_H
#define _SETUP_H

#define VECSIZE	0x200

extern void tlb_init(int cpu);
extern void per_cpu_trap_init(int cpu);
extern void set_handler(unsigned long offset, void *addr, unsigned long len);

#endif /* _SETUP_H */
