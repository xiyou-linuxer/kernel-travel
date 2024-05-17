// #include <asm/pt_regs.h>
#include <xkernel/kbuild.h>
#include <xkernel/thread.h>

void output_ptreg_defines(void);

void output_ptreg_defines(void)
{
	COMMENT("ARM64 pt_regs offsets.");
	BLANK();
}
