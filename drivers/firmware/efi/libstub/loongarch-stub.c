#include <asm/efi.h>
#include <linux/efi.h>
#include <linux/math.h>
#include <linux/string.h>

#include "efistub.h"

efi_status_t check_platform_features(void)
{
	return EFI_SUCCESS;
}

extern int kernel_asize;
extern int kernel_fsize;
extern int kernel_offset;
extern int kernel_entry;

efi_status_t handle_kernel_image(unsigned long *image_addr,
				 unsigned long *image_size,
				 unsigned long *reserve_addr,
				 unsigned long *reserve_size,
				 efi_loaded_image_t *image,
				 efi_handle_t image_handle)
{
	int nr_pages = round_up(kernel_asize, EFI_ALLOC_ALIGN) / EFI_PAGE_SIZE;
	efi_physical_addr_t kernel_addr = EFI_KIMG_PREFERRED_ADDRESS;
	efi_status_t status;

	status = efi_bs_call(allocate_pages,
			     EFI_ALLOCATE_ADDRESS,
			     EFI_LOADER_DATA,
			     nr_pages,
			     &kernel_addr);
	if (status != EFI_SUCCESS)
		return status;
	
	*image_addr = EFI_KIMG_PREFERRED_ADDRESS;
	*image_size = kernel_asize;

	memcpy((void*)EFI_KIMG_PREFERRED_ADDRESS,
		(void*)&kernel_offset - kernel_offset,
		kernel_fsize);
	
	return status;
}
