#ifndef _ASM_LOONGARCH_EFI_H
#define _ASM_LOONGARCH_EFI_H

#include <asm/addrspace.h>
#include <linux/types.h>
#include <linux/efi.h>

#define EFI_ALLOC_ALIGN		SZ_64K
#define EFI_RT_VIRTUAL_OFFSET	CSR_DMW0_BASE

#define EFI_KIMG_PREFERRED_ADDRESS	PHYSADDR(0x9000000000200000)

#endif /* _ASM_LOONGARCH_EFI_H */
