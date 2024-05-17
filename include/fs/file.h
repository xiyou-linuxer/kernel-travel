#ifndef _FILE_H
#define _FILE_H

#include <fs/fat32.h>
#include <xkernel/types.h>

#define NAME_MAX_LEN 256

int openat(int fd, u64 filename, int flags, mode_t mode);

// 以下位来自sys/stat.h，表示文件的st_mode
#define __S_IFMT 0170000 /* These bits determine file type.  */

/* File types. (文件类型值) */
#define __S_IFDIR 0040000  /* Directory.  */
#define __S_IFCHR 0020000  /* Character device.  */
#define __S_IFBLK 0060000  /* Block device.  */
#define __S_IFREG 0100000  /* Regular file.  */
#define __S_IFIFO 0010000  /* FIFO.  */
#define __S_IFLNK 0120000  /* Symbolic link.  */
#define __S_IFSOCK 0140000 /* Socket.  */

#define __S_ISTYPE(mode, mask) (((mode)&__S_IFMT) == (mask))

#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#define S_ISCHR(mode) __S_ISTYPE((mode), __S_IFCHR)
#define S_ISBLK(mode) __S_ISTYPE((mode), __S_IFBLK)
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)

/* Protection bits. (文件权限位) */
#define __S_ISUID 04000 /* Set user ID on execution.  */
#define __S_ISGID 02000 /* Set group ID on execution.  */
#define __S_ISVTX 01000 /* Save swapped text after use (sticky).  */
#define __S_IREAD 0400	/* Read by owner.  */
#define __S_IWRITE 0200 /* Write by owner.  */
#define __S_IEXEC 0100	/* Execute by owner.  */

// 文件被 owner, group, others 读、写、执行的权限位，默认全都有
#define S_IRUSR __S_IREAD  /* Read by owner.  */
#define S_IWUSR __S_IWRITE /* Write by owner.  */
#define S_IXUSR __S_IEXEC  /* Execute by owner.  */
/* Read, write, and execute by owner.  */
#define S_IRWXU (__S_IREAD | __S_IWRITE | __S_IEXEC)

#define S_IRGRP (S_IRUSR >> 3) /* Read by group.  */
#define S_IWGRP (S_IWUSR >> 3) /* Write by group.  */
#define S_IXGRP (S_IXUSR >> 3) /* Execute by group.  */
/* Read, write, and execute by group.  */
#define S_IRWXG (S_IRWXU >> 3)

#define S_IROTH (S_IRGRP >> 3) /* Read by others.  */
#define S_IWOTH (S_IWGRP >> 3) /* Write by others.  */
#define S_IXOTH (S_IXGRP >> 3) /* Execute by others.  */
/* Read, write, and execute by others.  */
#define S_IRWXO (S_IRWXG >> 3)

// 用于faccessat的mode参数
#define R_OK 4 /* Test for read permission.  */
#define W_OK 2 /* Test for write permission.  */
#define X_OK 1 /* Test for execute permission.  */
#define F_OK 0 /* Test for existence.  */

#endif
