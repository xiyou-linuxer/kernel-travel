#include <linux/efi.h>
#include <asm/efi.h>

#include "efistub.h"

#define EFI_RT_VIRTUAL_BASE	SZ_512M

/*
 * Some architectures map the EFI regions into the kernel's linear map using a
 * fixed offset.
 * 一些架构使用固定的偏移将EFI区域映射到内核的线性映射中。
 */
#ifndef EFI_RT_VIRTUAL_OFFSET
#define EFI_RT_VIRTUAL_OFFSET	0
#endif

static u64 virtmap_base = EFI_RT_VIRTUAL_BASE;
static bool flat_va_mapping = (EFI_RT_VIRTUAL_OFFSET != 0);

struct screen_info * __weak alloc_screen_info(void)
{
	return &screen_info;
}

void __weak free_screen_info(struct screen_info *si)
{
}

static u32 get_supported_rt_services(void)
{
	const efi_rt_properties_table_t *rt_prop_table;
	u32 supported = EFI_RT_SUPPORTED_ALL;

	rt_prop_table = get_efi_config_table(EFI_RT_PROPERTIES_TABLE_GUID);
	if (rt_prop_table)
		supported &= rt_prop_table->runtime_services_supported;

	return supported;
}

efi_status_t efi_alloc_virtmap(efi_memory_desc_t **virtmap,
			       unsigned long *desc_size, u32 *desc_ver)
{
	unsigned long size, mmap_key;
	efi_status_t status;

	/*
	 * Use the size of the current memory map as an upper bound for the
	 * size of the buffer we need to pass to SetVirtualAddressMap() to
	 * cover all EFI_MEMORY_RUNTIME regions.
	 */
	size = 0;
	status = efi_bs_call(get_memory_map, &size, NULL, &mmap_key, desc_size,
			     desc_ver);
	if (status != EFI_BUFFER_TOO_SMALL)
		return EFI_LOAD_ERROR;

	return efi_bs_call(allocate_pool, EFI_LOADER_DATA, size,
			   (void **)virtmap);
}

static void install_memreserve_table(void)
{
	struct linux_efi_memreserve *rsv;
	efi_guid_t memreserve_table_guid = LINUX_EFI_MEMRESERVE_TABLE_GUID;
	efi_status_t status;

	status = efi_bs_call(allocate_pool, EFI_LOADER_DATA, sizeof(*rsv),
			     (void **)&rsv);
	if (status != EFI_SUCCESS) {
		efi_err("Failed to allocate memreserve entry!\n");
		return;
	}

	rsv->next = 0;
	rsv->size = 0;
	atomic_set(&rsv->count, 0);

	status = efi_bs_call(install_configuration_table,
			     &memreserve_table_guid, rsv);
	if (status != EFI_SUCCESS)
		efi_err("Failed to install memreserve config table!\n");
}

efi_status_t efi_stub_common(efi_handle_t handle,
			     efi_loaded_image_t *image,
			     unsigned long image_addr,
			     char *cmdline_ptr)
{
	// struct screen_info *si = NULL;
	efi_status_t status;

	status = check_platform_features();
	if (status != EFI_SUCCESS)
		return status;

	efi_novamap |= !(get_supported_rt_services() &
			 EFI_RT_SUPPORTED_SET_VIRTUAL_ADDRESS_MAP);

	install_memreserve_table();

	status = efi_boot_kernel(handle, image, image_addr, cmdline_ptr);
	
	return status;
}

/*
 * efi_get_virtmap() - create a virtual mapping for the EFI memory map
 *
 * This function populates the virt_addr fields of all memory region descriptors
 * in @memory_map whose EFI_MEMORY_RUNTIME attribute is set. Those descriptors
 * are also copied to @runtime_map, and their total count is returned in @count.
 */
void efi_get_virtmap(efi_memory_desc_t *memory_map, unsigned long map_size,
		     unsigned long desc_size, efi_memory_desc_t *runtime_map,
		     int *count)
{
	u64 efi_virt_base = virtmap_base;
	efi_memory_desc_t *in, *out = runtime_map;
	int l;

	*count = 0;

	for (l = 0; l < map_size; l += desc_size) {
		u64 paddr, size;

		in = (void *)memory_map + l;
		if (!(in->attribute & EFI_MEMORY_RUNTIME))
			continue;

		paddr = in->phys_addr;
		size = in->num_pages * EFI_PAGE_SIZE;

		in->virt_addr = in->phys_addr + EFI_RT_VIRTUAL_OFFSET;
		if (efi_novamap) {
			continue;
		}

		/*
		 * Make the mapping compatible with 64k pages: this allows
		 * a 4k page size kernel to kexec a 64k page size kernel and
		 * vice versa.
		 */
		if (!flat_va_mapping) {

			paddr = round_down(in->phys_addr, SZ_64K);
			size += in->phys_addr - paddr;

			/*
			 * Avoid wasting memory on PTEs by choosing a virtual
			 * base that is compatible with section mappings if this
			 * region has the appropriate size and physical
			 * alignment. (Sections are 2 MB on 4k granule kernels)
			 */
			if (IS_ALIGNED(in->phys_addr, SZ_2M) && size >= SZ_2M)
				efi_virt_base = round_up(efi_virt_base, SZ_2M);
			else
				efi_virt_base = round_up(efi_virt_base, SZ_64K);

			in->virt_addr += efi_virt_base - paddr;
			efi_virt_base += size;
		}

		memcpy(out, in, desc_size);
		out = (void *)out + desc_size;
		++*count;
	}
}
