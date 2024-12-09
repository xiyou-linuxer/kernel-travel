#include <xkernel/printk.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <xkernel/init.h>
#include <xkernel/kernel.h>
#include <xkernel/atomic.h>
#include <asm/boot_param.h>
#include <asm/bootinfo.h>
#include <asm/fw.h>
#include <asm/loongarch.h>

struct boot_params *efi_bp;
struct loongsonlist_mem_map *loongson_mem_map;
struct loongsonlist_vbios *pvbios;
struct loongson_system_configuration loongson_sysconf;

static int get_bpi_version(void *signature)
{
	char data[8];
	int r, version = 0;

	memset(data, 0, 8);
	memcpy(data, signature + 4, 4);
	r = kstrtoint(data, 10, &version);

	if (r < 0 || version < BPI_VERSION_V1) {
		/**
		 * TODO: 实现panic
		 */
		printk("Fatal error, invalid BPI version: %d\n", version);
		while (1) ;
	}

	/**
	 * TODO: 解析flags
	 * if (version >= BPI_VERSION_V2)
	 * 	parse_flags(efi_bp->flags);
	 */

	return version;
}

void __init fw_init_environ(void)
{
	efi_bp = (struct boot_params *)_fw_envp;
	loongson_sysconf.bpi_ver = get_bpi_version(&efi_bp->signature);
	
	pr_info("BPI%d with boot flags %llx.\n", loongson_sysconf.bpi_ver, efi_bp->flags);

	printk("@@@@@: efi_bp = %p\n", efi_bp);
	printk("@@@@@: efi_bp->signature = %llx\n", efi_bp->signature);
	printk("@@@@@: flags = %llx\n", efi_bp->flags);
}
