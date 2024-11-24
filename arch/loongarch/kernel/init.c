#include <xkernel/init.h>
#include <asm/bootinfo.h>
#include <asm/fw.h>

void __init early_init(void)
{
	fw_init_cmdline();
	fw_init_environ();
}
