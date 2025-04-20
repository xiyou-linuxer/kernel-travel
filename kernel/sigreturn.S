#include <asm/asm.h>
.globl sigreturn_code
sigreturn_code:
#ifdef CONFIG_LOONGARCH
	li.d $a7,139
	syscall 0
#endif
