#ifndef _FS_SYS_H
#define _FS_SYS_H

#include <xkernel/types.h>

int sys_open(const char* pathname, int flags, mode_t mode);
int sys_write(int fd, const void* buf, unsigned int count);
int sys_read(int fd, void* buf, unsigned int count);
int sys_close(int fd);
int sys_mkdir(char* path, int mode);
char* sys_getcwd(char* buf, int size);
int sys_chdir(char* path);
int sys_unlink(char* pathname);
int sys_fstat(int fd, struct kstat* stat);
int sys_lseek(int fd, int offset, uint8_t whence);
int sys_dup(int oldfd);
int sys_dup2(uint32_t old_local_fd, uint32_t new_local_fd);
int sys_openat(int fd, const char* filename, int flags, mode_t mode);
int sys_mkdirat(int dirfd, const char* path, mode_t mode);
int sys_unlinkat(int dirfd, char* path, unsigned int flags);
int sys_mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data);
int sys_umount(const char* special);
#endif
