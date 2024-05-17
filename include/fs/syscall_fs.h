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
#endif