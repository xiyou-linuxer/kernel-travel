#ifndef _ASM_RISCV_MM_H
#define _ASM_RISCV_MM_H

#include <xkernel/linkage.h>
#include <xkernel/init.h>
#ifndef __ASSEMBLY__
asmlinkage void __init setup_vm(uintptr_t dtb_pa);
#endif /* __ASSEMBLY__ */

#endif