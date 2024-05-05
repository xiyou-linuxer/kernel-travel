#include <fs/fat32.h>
#include <fs/fd.h>
#include <fs/fd_device.h>
#include <fs/file.h>
#include <fs/pipe.h>
#include <fs/vfs.h>
#include <lib/log.h>
#include <lib/string.h>
#include <lib/transfer.h>
#include <lock/mutex.h>
#include <mm/kmalloc.h>
#include <proc/cpu.h>
#include <proc/interface.h>
#include <proc/sleep.h>
#include <proc/thread.h>
#include <sys/errno.h>
#include <sys/syscall_fs.h>
#include <mm/vmm.h>
#include <sys/syscall.h>

struct mutex mtx_fd;

// 下面的读写由mtx_fd保护
static uint fdBitMap[FDNUM / 32] = {0};
struct Fd fds[FDNUM];

int getDirentByFd(int fd, Dirent **dirent, int *kernFd);

/**
 * 加锁顺序：mtx_fd, fd->lock(sleeplock)
 */

// TODO: 实现初始化
void fd_init() {
	extern mutex_t mtx_file_load;
	mtx_init(&mtx_file_load, "kload", 0, MTX_SLEEP);
	mtx_init(&mtx_fd, "sys_fdtable", 1, MTX_SPIN | MTX_RECURSE);
}

void freeFd(uint i);

/**
 * @brief 分配一个文件描述符，保证文件描述符的内容是清空过的
 * @note 此为全局操作，需要获取fd锁
 */
int fdAlloc() {
	mtx_lock(&mtx_fd);

	uint i;
	for (i = 0; i < FDNUM; i++) {
		int index = i >> 5;
		int inner = i & 31;
		if ((fdBitMap[index] & (1 << inner)) == 0) {
			fdBitMap[index] |= 1 << inner;

			memset(&fds[i], 0, sizeof(struct Fd));
			fds[i].refcnt = 1;
			mtx_init(&fds[i].lock, "fd_lock", 1, MTX_SLEEP | MTX_RECURSE);

			mtx_unlock(&mtx_fd);
			return i;
		}
	}

	mtx_unlock(&mtx_fd);
	// 未找到空闲的系统fd
	warn("no free fd in kern fds\n");
	return -ENFILE;
}

/**
 * @brief 为某个kernFd添加一个新的进程引用
 * @param i 内核fd编号
 */
void cloneAddCite(uint i) {
	assert(i >= 0 && i < FDNUM);
	__sync_fetch_and_add(&fds[i].refcnt, 1); // 0 <= i < 1024
	__sync_synchronize();
}

/**
 * @brief 释放某个进程的文件描述符
 */
int closeFd(int fd) {
	int kernFd;
	if (fd < 0 || fd >= cur_proc_fs_struct()->rlimit_files_cur) {
		warn("close param fd is wrong, please check\n");
		return -EBADF;
	} else {
		if (cur_proc_fs_struct()->fdList[fd] < 0 ||
		    cur_proc_fs_struct()->fdList[fd] >= FDNUM) {
			warn("kern fd %d is wrong, please check\n", cur_proc_fs_struct()->fdList[fd]);
			return -EBADF;
		} else {
			kernFd = cur_proc_fs_struct()->fdList[fd];
			freeFd(kernFd);
			cur_proc_fs_struct()->fdList[fd] = -1;
			return 0;
		}
	}
}

/**
 * @brief 获取用户的空闲fd，标记后并返回。
 * 标记的目的是防止用户在获取fd后给fd数组赋值之前，再次调用此函数获取同一个fd
 * 加锁加的是td_fs_struct的锁
 * @return -EMFILE表示该进程可分配的文件描述符为空
 */
int alloc_ufd() {
	int ufd;
	mutex_t *lock = &cur_proc_fs_struct()->lock;
	mtx_lock(lock);

	// 检查用户可分配的Fd已分配完全
	for (int i = 0; i < cur_proc_fs_struct()->rlimit_files_cur; i++) {
		if (cur_proc_fs_struct()->fdList[i] == -1) {
			ufd = i;
			// 标识为FDNUM+1，防止用户再次使用该函数获取到一个fd，
			// 同时如果访问此fd也会因为大于FDNUM而报错
			cur_proc_fs_struct()->fdList[i] = FDNUM + 1;
			mtx_unlock(lock);
			return ufd;
		}
	}

	mtx_unlock(lock);
	warn("no spare fd for thread %s\n", cpu_this()->cpu_running->td_name);
	return -EMFILE;
}

/**
 * @brief 在用户fd层，释放用户fd，不关心其所对应的kernfd。
 * 如果释放一个空闲的ufd，会报错
 */
void free_ufd(int ufd) {
	mutex_t *lock = &cur_proc_fs_struct()->lock;
	mtx_lock(lock);

	assert(cur_proc_fs_struct()->fdList[ufd] != -1);
	cur_proc_fs_struct()->fdList[ufd] = -1;

	mtx_unlock(lock);
}

/**
 * @brief 将内核fd引用计数减一，如果引用计数归零，则回收
 */
void freeFd(uint i) {
	warn("Thread %s: free Fd: kernFd = %d\n", cpu_this()->cpu_running->td_name, i);
	assert (i >= 0 && i < FDNUM);
	Fd *fd = &fds[i];

	mtx_lock_sleep(&fd->lock);
	if (__sync_sub_and_fetch(&fd->refcnt, 1) == 0) {
		// Note 如果是file,不需要回收Dirent
		// Note 如果是pipe对应的fd关闭，则需要回收struct pipe对应的内存

		// 关闭fd对应的设备
		if (fd->fd_dev != NULL) { // 可能是新创建的kernFd，因为找不到文件而失败，被迫释放
			fd->fd_dev->dev_close(fd);
		}

		// 释放fd的资源
		fds[i].dirent = NULL;
		fds[i].pipe = NULL;
		fds[i].socket = NULL;
		fds[i].type = 0;
		fds[i].offset = 0;
		fds[i].flags = 0;

		mtx_unlock_sleep(&fd->lock); // 此时fd已不可能被查询到，故可以安心放锁
		memset(&fds[i], 0, sizeof(Fd));

		mtx_lock(&mtx_fd);
		int index = i >> 5;
		int inner = i & 31;
		fdBitMap[index] &= ~(1 << inner);
		mtx_unlock(&mtx_fd);
	} else {
		mtx_unlock_sleep(&fd->lock);
	}
}

/**
 * @brief 读取文件描述符，返回读取的字节数。允许只读一小部分
 */
int read(int fd, u64 buf, size_t count) {
	int kernFd;
	Fd *pfd;

	unwrap(getDirentByFd(fd, NULL, &kernFd));
	pfd = &fds[kernFd];
	mtx_lock_sleep(&pfd->lock);

	// 判断是否能读取
	if ((pfd->flags & O_ACCMODE) == O_WRONLY) {
		// socket: type 4
		warn("fd %d can not be read(type: %d, %d)\n", fd, pfd->type, pfd->socket ? pfd->socket->type : -10);

		mtx_unlock_sleep(&pfd->lock);
		return -EINVAL; // fd链接的对象不可读
	}

	// 处理dev_read
	int ret = pfd->fd_dev->dev_read(pfd, buf, count, pfd->offset);

	mtx_unlock_sleep(&pfd->lock);

	// debug
	// if (ret > 0) {
	// 	log(LEVEL_GLOBAL, "[read] %d bytes from fd %d\n", ret, fd);
	// 	if (ret > 0) {
	// 		char sbuf[1025];
	// 		copyInStr(buf, sbuf, MIN(1024, ret));
	// 		sbuf[MIN(1024, ret)] = '\0';
	// 		log(LEVEL_GLOBAL, "[read] content: %s\n", sbuf);
	// 	}
	// } else if (ret == 0) {
	// 	log(LEVEL_GLOBAL, "[read] %d bytes from fd %d\n", ret, fd);
	// } else {
	// 	log(LEVEL_GLOBAL, "[read] error %d from fd %d\n", ret, fd);
	// }

	return ret;
}

// position read，不维护偏移
size_t pread64(int fd, u64 buf, size_t count, off_t offset) {
	int kernFd;
	Fd *pfd;

	unwrap(getDirentByFd(fd, NULL, &kernFd));
	pfd = &fds[kernFd];
	mtx_lock_sleep(&pfd->lock);

	// 判断是否能读取
	if ((pfd->flags & O_ACCMODE) == O_WRONLY) {
		warn("fd %d can not be read\n", fd);

		mtx_unlock_sleep(&pfd->lock);
		return -EINVAL; // fd链接的对象不可读
	}

	// 处理dev_read
	off_t old_off = pfd->offset;
	int ret = pfd->fd_dev->dev_read(pfd, buf, count, offset);
	pfd->offset = old_off;

	mtx_unlock_sleep(&pfd->lock);
	return ret;
}

// readv是一个原子操作，单次调用readv读取内容不会被其他进程打断
// readv如果中间实际读取的长度小于需要读的长度，则可以中途返回
size_t readv(int fd, const struct iovec *iov, int iovcnt) {
	int kernFd;
	Fd *pfd;
	int len = 0, total = 0; // total表示总计读取的字节数
	struct iovec iov_temp;

	unwrap(getDirentByFd(fd, NULL, &kernFd));
	pfd = &fds[kernFd];
	mtx_lock_sleep(&pfd->lock);

	// 判断是否能读取
	if ((pfd->flags & O_ACCMODE) == O_WRONLY) {
		warn("fd can not be read\n");

		mtx_unlock_sleep(&pfd->lock);
		return -EINVAL;
	}

	// 处理dev_read
	for (int i = 0; i < iovcnt; i++) {
		// iov数组在用户态，需要copyIn读入
		copy_in(cur_proc_pt(), (u64)(&iov[i]), &iov_temp, sizeof(struct iovec));
		len = pfd->fd_dev->dev_read(pfd, (u64)iov_temp.iov_base, iov_temp.iov_len,
					    pfd->offset);
		if (len < 0) {
			// 读取出现问题，直接返回错误值
			mtx_unlock_sleep(&pfd->lock);
			return len;
		}
		total += len;

		if (len < iov_temp.iov_len) {
			// 读取结束，读不到更多数据，直接返回
			mtx_unlock_sleep(&pfd->lock);
			return total;
		}
	}

	mtx_unlock_sleep(&pfd->lock);
	return total;
}

size_t pwrite64(int fd, u64 buf, size_t count, off_t offset) {
	int kernFd;
	Fd *pfd;

	unwrap(getDirentByFd(fd, NULL, &kernFd));

	pfd = &fds[kernFd];

	mtx_lock_sleep(&pfd->lock);

	// 判断是否能写入
	if ((pfd->flags & O_ACCMODE) == O_RDONLY) {
		warn("fd can not be write\n");

		mtx_unlock_sleep(&pfd->lock);
		return -EINVAL;
	}

	// 处理dev_write
	off_t old_off = pfd->offset;
	int ret = pfd->fd_dev->dev_write(pfd, buf, count, offset);
	pfd->offset = old_off;

	mtx_unlock_sleep(&pfd->lock);
	return ret;
}

int write(int fd, u64 buf, size_t count) {
	int kernFd;
	Fd *pfd;

	unwrap(getDirentByFd(fd, NULL, &kernFd));

	pfd = &fds[kernFd];

	mtx_lock_sleep(&pfd->lock);

	// 判断是否能写入
	if ((pfd->flags & O_ACCMODE) == O_RDONLY) {
		warn("fd can not be write\n");

		mtx_unlock_sleep(&pfd->lock);
		return -EINVAL;
	}

	// 处理dev_write
	int ret = pfd->fd_dev->dev_write(pfd, buf, count, pfd->offset);
	mtx_unlock_sleep(&pfd->lock);

	// debug
	// if (ret >= 0) {
	// 	log(LEVEL_GLOBAL, "[write] %d bytes to fd %d\n", ret, fd);
	// 	if (ret > 0) {
	// 		char sbuf[1025];
	// 		copyInStr(buf, sbuf, MIN(1024, ret));
	// 		sbuf[MIN(1024, ret)] = '\0';
	// 		log(LEVEL_GLOBAL, "[write] content: %s\n", sbuf);
	// 	}
	// }

	return ret;
}

// writev是一个原子操作，单次调用writev读取内容不会被其他进程打断
// writev尽力写入所有数据
size_t writev(int fd, const struct iovec *iov, int iovcnt) {
	int kernFd;
	Fd *pfd;
	int len = 0, total = 0; // total表示总计读取的字节数
	struct iovec iov_temp;

	unwrap(getDirentByFd(fd, NULL, &kernFd));
	pfd = &fds[kernFd];
	mtx_lock_sleep(&pfd->lock);

	// 判断是否能写入
	if ((pfd->flags & O_ACCMODE) == O_RDONLY) {
		warn("fd can not be read\n");
		mtx_unlock_sleep(&pfd->lock);
		return -EINVAL;
	}

	// 处理dev_write
	for (int i = 0; i < iovcnt; i++) {
		copy_in(cur_proc_pt(), (u64)(&iov[i]), &iov_temp, sizeof(struct iovec));

		// len == 0，无需写入
		if (iov_temp.iov_len == 0) {
			continue;
		}
		len = pfd->fd_dev->dev_write(pfd, (u64)iov_temp.iov_base, iov_temp.iov_len,
					     pfd->offset);
		if (len < 0) {
			// 写入出现问题，直接返回错误值
			mtx_unlock_sleep(&pfd->lock);
			return len;
		}
		total += len;

		if (len < iov_temp.iov_len) {
			// 写入结束，写不进更多数据，直接返回
			// 一般的write会尽力写入所有给定的数据，但是如果是管道关闭，那么可能会只写入部分数据
			// 此时之后的数据也一定写不进去了，直接返回
			mtx_unlock_sleep(&pfd->lock);
			return total;
		}
	}

	mtx_unlock_sleep(&pfd->lock);
	return total;
}


size_t copy_file_range(int fd_in, off_t *off_in,
                        int fd_out, off_t *off_out,
                        size_t len, unsigned int flags) {
	off_t koff_in, koff_out;
	if (off_in != NULL) copyIn((u64)off_in, &koff_in, sizeof(off_t));
	if (off_out != NULL) copyIn((u64)off_out, &koff_out, sizeof(off_t));
	u64 r;

	if (ptLookup(cur_proc_pt(), U_KTEMPSPACE) == 0) {
		// 为用户态分配临时空间
		sys_map(U_KTEMPSPACE, 64 * PAGE_SIZE, PTE_R | PTE_W | PTE_U);
	}

	// 读取
	if (off_in == NULL) {
		// 维护偏移
		r = read(fd_in, U_KTEMPSPACE, len);
	} else {
		r = pread64(fd_in, U_KTEMPSPACE, len, koff_in);
		koff_in += r;
	}

	// 处理读取错误
	if (r <= 0) {
		return r;
	}

	len = r; // 约束复制量

	// 写入
	if (off_out == NULL) {
		// 维护偏移
		r = write(fd_out, U_KTEMPSPACE, len);
	} else {
		r = pwrite64(fd_out, U_KTEMPSPACE, len, koff_out);
		koff_out += r;
	}

	// 处理写入错误
	if (r < 0) {
		return r;
	}

	if (off_in != NULL) copyOut((u64)off_in, &koff_in, sizeof(off_t));
	if (off_out != NULL) copyOut((u64)off_out, &koff_out, sizeof(off_t));
	return len;
}

int dup(int fd) {
	int newFd = -1;
	int kernFd;

	unwrap(getDirentByFd(fd, NULL, &kernFd));
	if ((newFd = alloc_ufd()) < 0) {
		return newFd;
	}

	cur_proc_fs_struct()->fdList[newFd] = kernFd;
	cloneAddCite(kernFd);
	return newFd;
}

int dup3(int old, int new) {
	int copied;

	if (old == new)
		return -EINVAL;

	if (old < 0 || old >= cur_proc_fs_struct()->rlimit_files_cur) {
		warn("dup param old %d is wrong, please check\n", old);
		return -EBADF;
	}
	if (new < 0 || new >= cur_proc_fs_struct()->rlimit_files_cur) {
		warn("dup param newfd %d is wrong, please check\n", new);
		return -EBADF;
	}
	if (cur_proc_fs_struct()->fdList[new] >= 0 && cur_proc_fs_struct()->fdList[new] < FDNUM) {
		freeFd(cur_proc_fs_struct()->fdList[new]);
	} else if (cur_proc_fs_struct()->fdList[new] >= FDNUM) {
		warn("kern fd is wrong, please check\n");
		return -EBADF;
	}
	if (cur_proc_fs_struct()->fdList[old] < 0 || cur_proc_fs_struct()->fdList[old] >= FDNUM) {
		warn("kern fd is wrong, please check\n");
		return -EBADF;
	}
	copied = cur_proc_fs_struct()->fdList[old];
	cur_proc_fs_struct()->fdList[new] = copied;

	// kernFd引用计数加1
	cloneAddCite(copied);
	return new;
}

/**
 * @brief 检索fd对应的Dirent和kernFd，同时处理错误
 */
int getDirentByFd(int fd, Dirent **dirent, int *kernFd) {
	if (fd == AT_FDCWD) {
		if (dirent) {
			*dirent = get_cwd_dirent(cur_proc_fs_struct());
			if (*dirent == NULL) {
				warn("cwd fd is invalid: %s\n", cur_proc_fs_struct()->cwd);
				return -EBADF; // 需要检查所有调用此函数的上级函数是否有依赖
			}
			return 0;
		}
		// dirent无效时，由于AT_FDCWD是负数，应当继续下面的流程，直到报错
	}

	if (fd < 0 || fd >= cur_proc_fs_struct()->rlimit_files_cur) {
		warn("write param fd(%d) is wrong, please check\n", fd);
		return -EBADF;
	} else {
		if (cur_proc_fs_struct()->fdList[fd] < 0 ||
		    cur_proc_fs_struct()->fdList[fd] >= FDNUM) {
			warn("kern fd(%d) is wrong, please check\n",
			     cur_proc_fs_struct()->fdList[fd]);
			return -EBADF;
		} else {
			int kFd = cur_proc_fs_struct()->fdList[fd];
			if (kernFd)
				*kernFd = kFd;
			if (dirent)
				*dirent = fds[kFd].dirent;
			return 0;
		}
	}
}

/**
 * @brief 测试fd是否有效，有效返回kfd(不带锁)，无效返回NULL
 * 不接受fd为AT_CWD的情况，如果需要请移步getDirentByFd
 */
Fd *get_kfd_by_fd(int fd) {
	if (fd < 0 || fd >= cur_proc_fs_struct()->rlimit_files_cur) {
		warn("write param fd(%d) is wrong, please check\n", fd);
		return NULL;
	} else {
		if (cur_proc_fs_struct()->fdList[fd] < 0 ||
		    cur_proc_fs_struct()->fdList[fd] >= FDNUM) {
			warn("kern fd(%d) of fd %d is wrong, please check\n",
			     cur_proc_fs_struct()->fdList[fd], fd);
			return NULL;
		} else {
			int kfd = cur_proc_fs_struct()->fdList[fd];
			return &fds[kfd];
		}
	}
}

// 以下不涉及设备的读写访问

int getdents64(int fd, u64 buf, int len) {
	Dirent *dir, *file;
	int kernFd = 0, ret, offset;
	unwrap(getDirentByFd(fd, &dir, &kernFd));

	if (dir->type != DIRENT_DIR) {
		warn("getdents64: file %s is not a directory\n", dir->name);
		return -ENOTDIR;
	}

	DirentUser *direntUser = kmalloc(DIRENT_USER_SIZE);
	direntUser->d_ino = 0;
	direntUser->d_reclen = DIRENT_USER_SIZE;
	direntUser->d_type = dev_file;
	ret = dirGetDentFrom(dir, fds[kernFd].offset, &file, &offset, NULL);
	direntUser->d_off = offset;
	fds[kernFd].offset = offset;

	if (ret == 0) {
		// 读到了目录尾部，此时file为NULL
		warn("read dirents to the end! dir: %s\n", dir->name);
		direntUser->d_name[0] = '\0';
		copyOut(buf, direntUser, DIRENT_USER_SIZE);
		kfree(direntUser);
		return 0;
	} else {
		strncpy(direntUser->d_name, file->name, DIRENT_NAME_LENGTH);
		copyOut(buf, direntUser, DIRENT_USER_SIZE);
		dirent_dealloc(file);
		kfree(direntUser);
		return DIRENT_USER_SIZE;
	}
}

/**
 * @brief makeDirAtFd 在dirFd指定的目录下创建目录。请求进程仅创建目录，不持有创建目录的引用
 */
int makeDirAtFd(int dirFd, u64 path, int mode) {
	Dirent *dir;
	int ret;
	char name[MAX_NAME_LEN];

	unwrap(getDirentByFd(dirFd, &dir, NULL));
	copyInStr(path, name, MAX_NAME_LEN);

	log(LEVEL_GLOBAL, "make dir %s at %s\n", name, dir->name);
	ret = makeDirAt(dir, name, mode);
	return ret;
}

int linkAtFd(int oldFd, u64 pOldPath, int newFd, u64 pNewPath, int flags) {
	struct Dirent *oldDir, *newDir;
	char oldPath[MAX_NAME_LEN];
	char newPath[MAX_NAME_LEN];
	unwrap(getDirentByFd(oldFd, &oldDir, NULL));
	unwrap(getDirentByFd(newFd, &newDir, NULL));
	copyInStr(pOldPath, oldPath, MAX_NAME_LEN);
	copyInStr(pNewPath, newPath, MAX_NAME_LEN);
	return linkat(oldDir, oldPath, newDir, newPath);
}

int unLinkAtFd(int dirFd, u64 pPath) {
	struct Dirent *dir;
	char path[MAX_NAME_LEN];
	unwrap(getDirentByFd(dirFd, &dir, NULL));
	copyInStr(pPath, path, MAX_NAME_LEN);
	return unlinkat(dir, path);
}

// 以下两个stat的最终实现位于fat32/file.c的fileStat函数

/**
 * @brief 以fd为媒介获取文件状态
 */
int fileStatFd(int fd, u64 pkstat) {
	int kFd;
	unwrap(getDirentByFd(fd, NULL, &kFd));
	Fd *kernFd = &fds[kFd];

	mtx_lock_sleep(&kernFd->lock);
	unwrap(kernFd->fd_dev->dev_stat(kernFd, pkstat));
	mtx_unlock_sleep(&kernFd->lock);
	return 0;
}

/**
 * @brief 以fd+path为凭证获取文件状态
 * @note 需要获取和关闭dirent，在fileStat中会对dirent层加锁
 * @todo 需要处理flags为AT_NO_AUTOMOUNT或AT_EMPTY_PATH的情况
 */
int fileStatAtFd(int dirFd, u64 pPath, u64 pkstat, int flags) {
	Dirent *baseDir, *file;
	char path[MAX_NAME_LEN];
	int ret;
	unwrap(getDirentByFd(dirFd, &baseDir, NULL));
	copyInStr(pPath, path, MAX_NAME_LEN);

	log(LEVEL_GLOBAL, "fstat %s, dirFd is %d, flags = %x\n", path, dirFd, flags);

	if (flags & AT_SYMLINK_NOFOLLOW) {
		// 不跟随符号链接
		ret = get_file_raw(baseDir, path, &file);
	} else {
		ret = getFile(baseDir, path, &file);
	}
	if (ret < 0) {
		warn("can't find file %s at fd %d\n", path, dirFd);
		return ret;
	}
	struct kstat kstat;
	fileStat(file, &kstat);
	copyOut(pkstat, &kstat, sizeof(struct kstat));

	file_close(file);
	return 0;
}

/**
 * lseek() repositions the file offset of the open file description associated with the file
 descriptor fd to the argument offset according to the directive whence as follows:

       SEEK_SET
	      The file offset is set to offset bytes.

       SEEK_CUR
	      The file offset is set to its current location plus offset bytes.

       SEEK_END
	      The file offset is set to the size of the file plus offset bytes.

	Upon successful completion, lseek() returns the resulting offset location as measured in
 bytes from the begin‐ ning of the file.  On error, -errno is returned.
 */
off_t lseekFd(int fd, off_t offset, int whence) {
	// log(LEVEL_GLOBAL, "lseek fd %d, offset %ld, whence %d\n", fd, offset, whence);

	int kFd_num;
	int ret = 0;
	off_t new_off = 0;
	struct kstat kstat;
	unwrap(getDirentByFd(fd, NULL, &kFd_num));
	Fd *kernFd = &fds[kFd_num];

	mtx_lock_sleep(&kernFd->lock);
	if (whence == SEEK_SET || whence == SEEK_DATA) {
		new_off = offset;
	} else if (whence == SEEK_CUR) {
		new_off = kernFd->offset + offset;
	} else if (whence == SEEK_END) {
		// 只有磁盘文件才能获取到文件的大小
		if (kernFd->type != dev_file) {
			warn("only file can use SEEK_END\n");
			ret = -ESPIPE;
		} else {
			fileStat(kernFd->dirent, &kstat);
			new_off = kstat.st_size + offset;
		}
	} else if (whence == SEEK_HOLE) {
		fileStat(kernFd->dirent, &kstat);
		// 文件中没有任何空洞，设置offset为文件尾部
		new_off = kstat.st_size;
	} else {
		warn("unknown lseek whence %d\n", whence);
		ret = -EINVAL;
	}

	if (ret < 0) {
		mtx_unlock_sleep(&kernFd->lock);
		return ret;
	} else if (new_off < 0) {
		warn("lseek offset %d is negative\n", new_off);
		mtx_unlock_sleep(&kernFd->lock);
		return -EINVAL;
	} else {
		kernFd->offset = new_off;
		mtx_unlock_sleep(&kernFd->lock);
		return new_off;
	}
}

int faccessatFd(int dirFd, u64 pPath, int mode, int flags) {
	Dirent *dir;
	char path[MAX_NAME_LEN];
	unwrap(getDirentByFd(dirFd, &dir, NULL));
	copyInStr(pPath, path, MAX_NAME_LEN);
	return faccessat(dir, path, mode, flags);
}
