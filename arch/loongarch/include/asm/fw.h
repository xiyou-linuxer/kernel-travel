#ifndef _ASM_FW_H
#define _ASM_FW_H

#include <asm/addrspace.h>

extern int fw_argc;
extern long *_fw_argv, *_fw_envp;

#define fw_argv(index)		((char *)TO_CACHE((long)_fw_argv[(index)]))
#define fw_envp(index)		((char *)TO_CACHE((long)_fw_envp[(index)]))

extern void fw_init_cmdline(void);

#endif /* _ASM_FW_H */
