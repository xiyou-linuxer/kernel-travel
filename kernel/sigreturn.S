#include <asm/asm.h>
.globl sigreturn_code
sigreturn_code:
	li.d $a7,139
	syscall 0
