#ifndef __XKERNEL_MMAP_H__
#define __XKERNEL_MMAP_H__
#include <xkernel/memory.h>
#include <fs/fd.h>
#include <fs/syscall_fs.h>
#include "xkernel/types.h"

#define MAP_FILE	0
#define MAP_SHARED	0x01		/* Share changes */
#define MAP_PRIVATE	0x02		/* Changes are private */
#define MAP_SHARED_VALIDATE 0x03	/* share + validate extension flags */

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

void* sys_mmap(void* addr, size_t len,
			int prot, int flags,
			int fd, off_t pgoff);
int sys_munmap(void *start, size_t len);
void test_mmap(void);


#endif