#include <linux/efi.h>
#include <asm/efi.h>
#include <stdarg.h>

#include "efistub.h"

bool efi_novamap;

/**
 * get_efi_config_table() - retrieve UEFI configuration table
 * @guid:	GUID of the configuration table to be retrieved
 * Return:	pointer to the configuration table or NULL
 */
void *get_efi_config_table(efi_guid_t guid)
{
	unsigned long tables = efi_table_attr(efi_system_table, tables);
	int nr_tables = efi_table_attr(efi_system_table, nr_tables);
	int i;

	for (i = 0; i < nr_tables; i++) {
		efi_config_table_t *t = (void *)tables;

		if (efi_guidcmp(t->guid, guid) == 0)
			return efi_table_attr(t, table);

		tables += efi_is_native() ? sizeof(efi_config_table_t)
					  : sizeof(efi_config_table_32_t);
	}
	return NULL;
}

efi_status_t efi_exit_boot_services(void *handle, void *priv,
				    efi_exit_boot_map_processing priv_func)
{
	struct efi_boot_memmap *map;
	efi_status_t status;

	status = efi_get_memory_map(&map, true);
	if (status != EFI_SUCCESS)
		return status;

	status = priv_func(map, priv);
	if (status != EFI_SUCCESS) {
		efi_bs_call(free_pool, map);
		return status;
	}

	status = efi_bs_call(exit_boot_services, handle, map->map_key);

	if (status == EFI_INVALID_PARAMETER) {
		map->map_size = map->buff_size;
		status = efi_bs_call(get_memory_map,
				     &map->map_size,
				     &map->map,
				     &map->map_key,
				     &map->desc_size,
				     &map->desc_ver);

		/* exit_boot_services() was called, thus cannot free */
		if (status != EFI_SUCCESS)
			return status;

		status = priv_func(map, priv);
		/* exit_boot_services() was called, thus cannot free */
		if (status != EFI_SUCCESS)
			return status;

		status = efi_bs_call(exit_boot_services, handle, map->map_key);
	}

	return status;
}
