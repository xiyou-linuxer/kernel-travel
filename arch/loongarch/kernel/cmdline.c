#include <xkernel/init.h>
#include <xkernel/string.h>
#include <xkernel/printk.h>
#include <asm/fw.h>
#include <asm/bootinfo.h>
#include <asm/early_ioremap.h>

int fw_argc;
long *_fw_argv, *_fw_envp;

void __init fw_init_cmdline(void)
{
	int i;

	fw_argc = fw_arg0;
	_fw_argv = (long *)early_memremap_ro(fw_arg1, SZ_16K);
	_fw_envp = (long *)early_memremap_ro(fw_arg2, SZ_64K);

	arcs_cmdline[0] = '\0';
	for (i = 1; i < fw_argc; i++) {
		strlcat(arcs_cmdline, fw_argv(i), 512);
		if (i < (fw_argc - 1))
			strlcat(arcs_cmdline, " ", 512);
	}

	pr_info("cmdline:%s\n", arcs_cmdline);
}
