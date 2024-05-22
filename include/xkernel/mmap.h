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

#define MAP_GROWSDOWN	0x0100		/* stack-like segment */
#define MAP_DENYWRITE	0x0800		/* ETXTBSY */
#define MAP_EXECUTABLE	0x1000		/* mark it as an executable */
#define MAP_LOCKED	0x2000		/* pages are locked */
#define MAP_NORESERVE	0x4000		/* don't check for reservations */
#define MAP_POPULATE	0x8000		/* populate (prefault) pagetables */
#define MAP_NONBLOCK	0x10000		/* do not block on IO */

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#define TASK_UNMAPPED_BASE	(0x40000000UL)
#endif

extern unsigned long do_mmap_pgoff(struct file *file, unsigned long addr,
	unsigned long len, unsigned long prot,
	unsigned long flag, unsigned long pgoff);

// static inline
unsigned long do_mmap(struct file *file, unsigned long addr,
	unsigned long len, unsigned long prot,
	unsigned long flag, unsigned long offset);
// {
// 	unsigned long ret = -1;
// 	// if ((offset + PAGE_ALIGN(len)) < offset)
// 		// goto out;
// 	if (!(offset & ~PAGE_MASK))
// 		ret = do_mmap_pgoff(file, addr, len, prot, flag, offset >> PAGE_SHIFT);
// out:
// 	return ret;
// }

void* sys_mmap(void* addr, size_t len,
			int prot, int flags,
			int fd, off_t pgoff);
int sys_munmap(void *start, size_t len);
void test_mmap(void);

unsigned long
get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags);

#endif