#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <xkernel/init.h>
#include <asm/boot_param.h>
#include <asm/bootinfo.h>
#include <asm/fw.h>
#include <asm/loongarch.h>

struct boot_params *efi_bp;
struct loongsonlist_mem_map *loongson_mem_map;
struct loongsonlist_vbios *pvbios;
struct loongson_system_configuration loongson_sysconf;

void __init fw_init_environ(void)
{
	efi_bp = (struct boot_params *)_fw_envp;
	printk("@@@@@: efi_bp = %p\n", efi_bp);
	printk("@@@@@: efi_bp->signature = %llx\n", efi_bp->signature);
	printk("@@@@@: flags = %llx\n", efi_bp->flags);
}
