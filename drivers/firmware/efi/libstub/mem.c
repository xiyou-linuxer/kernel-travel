#include <linux/efi.h>
#include <asm/efi.h>

#include "efistub.h"

/**
 * efi_get_memory_map() - get memory map
 * @map:		pointer to memory map pointer to which to assign the
 *			newly allocated memory map
 * @install_cfg_tbl:	whether or not to install the boot memory map as a
 *			configuration table
 *
 * Retrieve the UEFI memory map. The allocated memory leaves room for
 * up to EFI_MMAP_NR_SLACK_SLOTS additional memory map entries.
 *
 * Return:	status code
 */
efi_status_t efi_get_memory_map(struct efi_boot_memmap **map,
				bool install_cfg_tbl)
{
	int memtype = install_cfg_tbl ? EFI_ACPI_RECLAIM_MEMORY
				      : EFI_LOADER_DATA;
	efi_guid_t tbl_guid = LINUX_EFI_BOOT_MEMMAP_GUID;
	struct efi_boot_memmap *m, tmp;
	efi_status_t status;
	unsigned long size;

	tmp.map_size = 0;
	status = efi_bs_call(get_memory_map, &tmp.map_size, NULL, &tmp.map_key,
			     &tmp.desc_size, &tmp.desc_ver);
	if (status != EFI_BUFFER_TOO_SMALL)
		return EFI_LOAD_ERROR;

	size = tmp.map_size + tmp.desc_size * EFI_MMAP_NR_SLACK_SLOTS;
	status = efi_bs_call(allocate_pool, memtype, sizeof(*m) + size,
			     (void **)&m);
	if (status != EFI_SUCCESS)
		return status;

	if (install_cfg_tbl) {
		/*
		 * Installing a configuration table might allocate memory, and
		 * this may modify the memory map. This means we should install
		 * the configuration table first, and re-install or delete it
		 * as needed.
		 */
		status = efi_bs_call(install_configuration_table, &tbl_guid, m);
		if (status != EFI_SUCCESS)
			goto free_map;
	}

	m->buff_size = m->map_size = size;
	status = efi_bs_call(get_memory_map, &m->map_size, m->map, &m->map_key,
			     &m->desc_size, &m->desc_ver);
	if (status != EFI_SUCCESS)
		goto uninstall_table;

	*map = m;
	return EFI_SUCCESS;

uninstall_table:
	if (install_cfg_tbl)
		efi_bs_call(install_configuration_table, &tbl_guid, NULL);
free_map:
	efi_bs_call(free_pool, m);
	return status;
}
