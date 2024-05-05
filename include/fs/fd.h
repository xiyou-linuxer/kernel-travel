#ifndef _FD_H
#define _FD_H

#include <fs/fat32.h>
#include <fs/fs.h>
#include <sync.h>
#include <linux/thread.h>
#include <linux/types.h>

#define FDNUM 1024

typedef struct FdDev FdDev;
typedef struct Socket Socket;

typedef struct Fd {
	// 保证每个fd的读写不并发
	mutex_t lock;

	Dirent *dirent;
	struct Pipe *pipe;
	int type;
	uint offset;
	uint flags;
	struct kstat stat;
	FdDev *fd_dev;

	u32 refcnt; // 引用计数
	Socket *socket;
} Fd;

typedef struct DirentUser {
	uint64 d_ino;		 // 索引结点号
	i64 d_off;		 // 下一个dirent到文件首部的偏移
	unsigned short d_reclen; // 当前dirent的长度
	unsigned char d_type;	 // 文件类型
	char d_name[];		 // 文件名
} DirentUser;

#define DIRENT_USER_SIZE 128
#define DIRENT_USER_OFFSET_NAME ((u64) & (((DirentUser *)0)->d_name))
#define DIRENT_NAME_LENGTH (DIRENT_USER_SIZE - DIRENT_USER_OFFSET_NAME)

extern struct Fd fds[FDNUM];
extern uint citesNum[FDNUM];

#define dev_file 1
#define dev_pipe 2
#define dev_console 3
#define dev_socket 4

#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR 0x002
#define O_ACCMODE 0x003
// 0100
#define O_CREATE 0x40
// TODO CREATE标志位存疑

// 来自bits/fcntl-linux.h
#ifndef O_EXCL
#define O_EXCL 0200 /* Not fcntl.  */
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0400 /* Not fcntl.  */
#endif
#ifndef O_TRUNC
#define O_TRUNC 01000 /* Not fcntl.  */
#endif
#ifndef O_APPEND
#define O_APPEND 02000
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif
#ifndef O_SYNC
#define O_SYNC 04010000
#endif
#define O_FSYNC O_SYNC
#ifndef O_ASYNC
#define O_ASYNC 020000
#endif
#ifndef __O_LARGEFILE
#define __O_LARGEFILE 0100000
#endif

#ifndef __O_DIRECTORY
#define __O_DIRECTORY 0200000
#endif
#ifndef __O_NOFOLLOW
#define __O_NOFOLLOW 0400000
#endif
#ifndef __O_CLOEXEC
#define __O_CLOEXEC 02000000
#endif
#ifndef __O_DIRECT
#define __O_DIRECT 040000
#endif
#ifndef __O_NOATIME
#define __O_NOATIME 01000000
#endif
#ifndef __O_PATH
#define __O_PATH 010000000
#endif
#ifndef __O_DSYNC
#define __O_DSYNC 010000
#endif
#ifndef __O_TMPFILE
#define __O_TMPFILE (020000000 | __O_DIRECTORY)
#endif

#define AT_FDCWD -100

struct iovec;

void fd_init();
int fdAlloc();
int alloc_ufd();
void free_ufd(int ufd);
int closeFd(int fd);
void cloneAddCite(uint i);

int read(int fd, u64 buf, size_t count);
int write(int fd, u64 buf, size_t count);
size_t pread64(int fd, u64 buf, size_t count, off_t offset);
size_t pwrite64(int fd, u64 buf, size_t count, off_t offset);
size_t readv(int fd, const struct iovec *iov, int iovcnt);
size_t writev(int fd, const struct iovec *iov, int iovcnt);

int dup(int fd);
int dup3(int old, int new);
void freeFd(uint i);
int getdents64(int fd, u64 buf, int len);
int makeDirAtFd(int dirFd, u64 path, int mode);
int linkAtFd(int oldFd, u64 pOldPath, int newFd, u64 pNewPath, int flags);
int unLinkAtFd(int dirFd, u64 pPath);
int fileStatFd(int fd, u64 pkstat);
int getDirentByFd(int fd, Dirent **dirent, int *kernFd);
Fd *get_kfd_by_fd(int fd);
int fileStatAtFd(int dirFd, u64 pPath, u64 pkstat, int flags);
off_t lseekFd(int fd, off_t offset, int whence);
int faccessatFd(int dirFd, u64 pPath, int mode, int flags);

// new
size_t copy_file_range(int fd_in, off_t *off_in,
                        int fd_out, off_t *off_out,
                        size_t len, unsigned int flags);

#endif
