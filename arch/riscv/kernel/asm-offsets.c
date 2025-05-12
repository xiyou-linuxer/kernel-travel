#include <asm/processor.h>
#include <xkernel/kbuild.h>
#include <xkernel/thread.h>
#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
void asm_offsets(void);

void asm_offsets(void)
{
    DEFINE(PT_SIZE_ON_STACK, ALIGN(sizeof(struct pt_regs), STACK_ALIGN));
}