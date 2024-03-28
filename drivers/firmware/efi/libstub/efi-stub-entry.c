#include <linux/efi.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/printk.h>

#include "efistub.h"

/* Is this type a native word size -- useful for atomic operations */
#define __native_word(t) \
	(sizeof(t) == sizeof(char) || sizeof(t) == sizeof(short) || \
	 sizeof(t) == sizeof(int) || sizeof(t) == sizeof(long))

# define __compiletime_assert(condition, msg, prefix, suffix) do { } while (0)

#define _compiletime_assert(condition, msg, prefix, suffix) \
	__compiletime_assert(condition, msg, prefix, suffix)

#define compiletime_assert(condition, msg) \
	_compiletime_assert(condition, msg, __compiletime_assert_, __COUNTER__)

#define compiletime_assert_rwonce_type(t)					\
	compiletime_assert(__native_word(t) || sizeof(t) == sizeof(long long),	\
		"Unsupported access size for {READ,WRITE}_ONCE().")

#define __WRITE_ONCE(x, val)						\
do {									\
	*(volatile typeof(x) *)&(x) = (val);				\
} while (0)

#define WRITE_ONCE(x, val)						\
do {									\
	compiletime_assert_rwonce_type(x);				\
	__WRITE_ONCE(x, val);						\
} while (0)

efi_status_t __efiapi efi_pe_entry(efi_handle_t handle, efi_system_table_t *systab)
{	
	efi_loaded_image_t *image;
	efi_status_t status;
	unsigned long image_addr;
	unsigned long image_size = 0;
	/* addr/point and size pairs for memory management*/
	char *cmdline_ptr = NULL;
	efi_guid_t loaded_image_proto = LOADED_IMAGE_PROTOCOL_GUID;
	unsigned long reserve_addr = 0;
	unsigned long reserve_size = 0;

	// efi_system_table = systab;
	__WRITE_ONCE(efi_system_table, systab);

	/* Check if we were booted by the EFI firmware */
	if (efi_system_table->hdr.signature != EFI_SYSTEM_TABLE_SIGNATURE)
		return EFI_INVALID_PARAMETER;
	
	/*
	 * Get a handle to the loaded image protocol.  This is used to get
	 * information about the running image, such as size and the command
	 * line.
	 */
	status = efi_bs_call(handle_protocol, handle, &loaded_image_proto,
			     (void *)&image);
	if (status != EFI_SUCCESS)
		return status;

	status = handle_kernel_image(&image_addr, &image_size,
				     &reserve_addr,
				     &reserve_size,
				     image, handle);
	if (status != EFI_SUCCESS) {
		efi_err("Failed to relocate kernel\n");
		return status;
	}

	efi_info("Booting XOS kernel... \n");

	status = efi_stub_common(handle, image, image_addr, cmdline_ptr);

	return status;
}
