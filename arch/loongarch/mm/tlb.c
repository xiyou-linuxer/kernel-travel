#include <asm/tlb.h>

void __update_tlb(void)
{
	tlb_read();
}