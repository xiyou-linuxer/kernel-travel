#include <fs/fd.h>
#include <fs/file.h>
#include <fs/file_time.h>
#include <fs/pipe.h>
#include <fs/thread_fs.h>
#include <fs/vfs.h>
#include <fs/buf.h>
#include <lib/log.h>
#include <lib/string.h>
#include <lib/transfer.h>
#include <mm/memlayout.h>
#include <mm/vmm.h>
#include <proc/cpu.h>
#include <proc/interface.h>
#include <proc/thread.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/syscall_fs.h>
#include <sys/time.h>
#include <dev/timer.h>
#include <fs/pipe.h>
#include <proc/tsleep.h>
#include <fs/socket.h>
#include <fs/console.h>

static inline int check_pselect_r(int fd);
static inline int check_pselect_w(int fd);

int sys_write(int fd, u64 buf, size_t count) {
	return write(fd, buf, count);
}

int sys_read(int fd, u64 buf, size_t count) {
	return read(fd, buf, count);
}

int sys_openat(int fd, u64 filename, int flags, mode_t mode) {
	return openat(fd, filename, flags, mode);
}

int sys_fchmod(int fd, mode_t mode) {
	Dirent *file;
	unwrap(getDirentByFd(fd, &file, NULL));
	log(999, "fchmod: fd = %d, mode = %x, file = %s\n", fd, mode, file->name);
	file->mode = mode;
	return 0;
}

int sys_close(int fd) {
	warn("ufd: %d\n", fd);
	return closeFd(fd);
}

// 关闭一个socket fd
// TODO: 未来可能要根据
int sys_shutdown(int fd, int how) {
	return shutdown(fd, how);
}

int sys_dup(int fd) {
	return dup(fd);
}

int sys_dup3(int fd_old, int fd_new) {
	return dup3(fd_old, fd_new);
}

u64 sys_getcwd(u64 buf, int size) {
	void *kptr = cur_proc_fs_struct()->cwd;
	copyOut(buf, kptr, MIN(256, size));
	return buf;
}

int sys_pipe2(u64 pfd) {
	int fd[2];
	int ret = pipe(fd);
	if (ret < 0) {
		return ret;
	} else {
		copyOut(pfd, fd, sizeof(fd));
		return ret;
	}
}

int sys_chdir(u64 path) {
	char kbuf[MAX_NAME_LEN], new_cwd[MAX_NAME_LEN];
	copyInStr(path, kbuf, MAX_NAME_LEN);
	thread_fs_t *thread_fs = cur_proc_fs_struct();

	if (kbuf[0] == '/') {
		// 绝对路径
		strncpy(new_cwd, kbuf, MAX_NAME_LEN);
		assert(strlen(new_cwd) + 3 < MAX_NAME_LEN);
	} else {
		// 相对路径
		// 保证操作之前cwd以"/"结尾
		strncpy(new_cwd, thread_fs->cwd, MAX_NAME_LEN);
		strcat(new_cwd, "/");
		assert(strlen(new_cwd) + strlen(kbuf) + 3 < MAX_NAME_LEN);
		strcat(new_cwd, kbuf);
	}

	// 检查new_cwd是否指向有效的目录
	Dirent *cwd_dirent;
	int ret = getFile(NULL, new_cwd, &cwd_dirent);
	if (ret < 0) {
		return ret;
	} else if (!is_directory(&cwd_dirent->raw_dirent)) {
		file_close(cwd_dirent);
		return -ENOTDIR;
	}

	if (thread_fs->cwd_dirent != NULL) {
		file_close(thread_fs->cwd_dirent);
	}

	strncpy(thread_fs->cwd, new_cwd, MAX_NAME_LEN);
	thread_fs->cwd_dirent = cwd_dirent;
	return 0;
}

int sys_mkdirat(int dirFd, u64 path, int mode) {
	return makeDirAtFd(dirFd, path, mode);
}

/*int sys_mount(u64 special, u64 dir, u64 fstype, u64 flags, u64 data) {
	char specialStr[MAX_NAME_LEN];
	char dirPath[MAX_NAME_LEN];

	// 1. 将special和dir加载到字符串数组中
	copyInStr(special, specialStr, MAX_NAME_LEN);
	copyInStr(dir, dirPath, MAX_NAME_LEN);

	// 2. 计算cwd，如果dir不是绝对路径，则是相对于cwd
	Dirent *cwd = get_cwd_dirent(cur_proc_fs_struct());

	// 3. 挂载
	return mount_fs(specialStr, cwd, dirPath);
}*/

/*int sys_umount(u64 special, u64 flags) {
	char specialStr[MAX_NAME_LEN];

	// 1. 将special和dir加载到字符串数组中
	copyInStr(special, specialStr, MAX_NAME_LEN);

	// 2. 计算cwd，如果dir不是绝对路径，则是相对于cwd
	Dirent *cwd = get_cwd_dirent(cur_proc_fs_struct());

	// 3. 解除挂载
	return umount_fs(specialStr, cwd);
}*/

int sys_linkat(int oldFd, u64 pOldPath, int newFd, u64 pNewPath, int flags) {
	return linkAtFd(oldFd, pOldPath, newFd, pNewPath, flags);
}

int sys_unlinkat(int dirFd, u64 pPath) {
	return unLinkAtFd(dirFd, pPath);
}

int sys_getdents64(int fd, u64 buf, int len) {
	return getdents64(fd, buf, len);
}

int sys_fstat(int fd, u64 pkstat) {
	return fileStatFd(fd, pkstat);
}

#define TIOCGWINSZ 0x5413
struct WinSize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
} winSize = {24, 80, 0, 0};

// 调整io，一般用来获取窗口尺寸
int sys_ioctl(int fd, u64 request, u64 data) {
	if (request == TIOCGWINSZ) {
		copyOut(data, &winSize, sizeof(winSize));
	}
	// 否则不做任何操作
	return 0;
}

size_t sys_readv(int fd, const struct iovec *iov, int iovcnt) {
	return readv(fd, iov, iovcnt);
}

size_t sys_writev(int fd, const struct iovec *iov, int iovcnt) {
	return writev(fd, iov, iovcnt);
}

size_t sys_copy_file_range(int fd_in, off_t *off_in,
                        int fd_out, off_t *off_out,
                        size_t len, unsigned int flags) {
	return copy_file_range(fd_in, off_in, fd_out, off_out, len, flags);
}

size_t sys_getrandom(u64 buf, size_t buflen, unsigned int flags) {
	u8 prime = 251;
	u64 now = time_rtc_clock();
	for (int i = 0; i < buflen; i++) {
		u8 ch = (now * (u64)i + i * i) % prime;
		copyOut(buf, &ch, 1);
	}
	return buflen;
}

int sys_fstatat(int dirFd, u64 pPath, u64 pkstat, int flags) {
	return fileStatAtFd(dirFd, pPath, pkstat, flags);
}

// 因为所有write默认都会写回磁盘，所以此处可以什么也不做
int sys_fsync(int fd) {
	return 0;
}

off_t sys_lseek(int fd, off_t offset, int whence) {
	return lseekFd(fd, offset, whence);
}

int sys_faccessat(int dirFd, u64 pPath, int mode, int flags) {
	return faccessatFd(dirFd, pPath, mode, flags);
}

/**
 * @brief 原型：int ppoll(struct pollfd *fds, nfds_t nfds,
	       const struct timespec *tmo_p, const sigset_t *sigmask);
 * 等待其中一个文件描述符就绪
 * 目前的处理策略：标记所有有效fd为就绪，无视timeout和sigmask，立即返回(TODO)
 * @return fds数组中revents非0的个数。其中fds数组中，
 * revents返回events标定事件的子集或者三种特殊值：POLLERR, POLLHUP, POLLNVAL
 */
int sys_ppoll(u64 p_fds, int nfds, u64 tmo_p, u64 sigmask) {
	struct pollfd poll_fd;
	struct timespec tmo;
	int ret = 0;
	if (tmo_p) {
		copyIn(tmo_p, &tmo, sizeof(tmo));
		if (TS_USEC(tmo) != 0) tsleep(&tmo, NULL, "ppoll", time_mono_us() + TS_USEC(tmo));
	}

	while (1) {
		ret = 0;
		for (int i = 0; i < nfds; i++) {
			u64 cur_fds = p_fds + i * sizeof(poll_fd);
			copyIn(cur_fds, &poll_fd, sizeof(poll_fd));
			poll_fd.revents = 0;

			if (poll_fd.fd < 0) {
				poll_fd.revents = 0;
			} else {
				// 目前只处理POLLIN和POLLOUT两种等待事件
				if (poll_fd.events & POLLIN) {
					int r = check_pselect_r(poll_fd.fd);
					if (r < 0) return r;
					if (r) poll_fd.revents |= POLLIN;
				}
				if (poll_fd.events & POLLOUT) {
					int r = check_pselect_w(poll_fd.fd);
					if (r < 0) return r;
					if (r) poll_fd.revents |= POLLOUT;
				}
			}
			copyOut(cur_fds, &poll_fd, sizeof(poll_fd));

			if (poll_fd.revents != 0)
				ret += 1;
		}

		if (ret || tmo_p) {
			break;
		}

		// if (!tmo_p) {
		// 	tsleep(&tmo, NULL, "ppoll", 100);
		// }
	}
	return ret;
}

/**
 * @brief 检查fd是否可以读取。可以返回1，不可以返回0，失败返回负数
 */
static inline int check_pselect_r(int fd) {
	Fd *kfd = get_kfd_by_fd(fd);
	int ret;
	if (kfd == NULL) {
		return -EBADF;
	}
	mtx_lock_sleep(&kfd->lock);

	// 目前只处理socket和其他
	if (kfd->type == dev_socket) {
		ret = socket_read_check(kfd);
	} else if (kfd->type == dev_pipe) {
		ret = pipe_check_read(kfd->pipe);
	} else if (kfd->type == dev_console) {
		ret = console_check_read();
	} else {
		ret = 1;
	}
	mtx_unlock_sleep(&kfd->lock);
	return ret;
}

/**
 * @brief 检查fd是否可以写入。可以返回1，不可以返回0，失败返回负数
 */
static inline int check_pselect_w(int fd) {
	Fd *kfd = get_kfd_by_fd(fd);
	int ret;
	if (kfd == NULL) {
		warn("pselect6: fd %d not found\n", fd);
		return -EBADF;
	}
	mtx_lock_sleep(&kfd->lock);

	// 目前只处理socket,pipe,其他
	if (kfd->type == dev_socket) {
		ret = socket_write_check(kfd);
	} else if (kfd->type == dev_pipe) {
		ret = pipe_check_write(kfd->pipe);
	} else if (kfd->type == dev_console) {
		ret = console_check_write();
	} else {
		ret = 1;
	}
	mtx_unlock_sleep(&kfd->lock);
	return ret;
}

/**
 * 原型：int pselect6(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, const struct timespec *timeout,
                   const sigset_t *sigmask) {
 */
int sys_pselect6(int nfds, u64 p_readfds, u64 p_writefds, u64 p_exceptfds, u64 p_timeout,
				u64 sigmask) {
	int fd, r;
	int func_ret = 0;
	u64 timeout_us;

	fd_set readfds, writefds, exceptfds;
	fd_set readfds_cur, writefds_cur, exceptfds_cur;
	memset(&readfds, 0, sizeof(readfds));
	memset(&writefds, 0, sizeof(writefds));
	memset(&exceptfds, 0, sizeof(exceptfds));

	struct timespec timeout;
	memset(&timeout, 0, sizeof(timeout));
	if (p_readfds) copyIn(p_readfds, &readfds, sizeof(readfds));
	if (p_writefds) copyIn(p_writefds, &writefds, sizeof(writefds));
	if (p_exceptfds) copyIn(p_exceptfds, &exceptfds, sizeof(exceptfds));
	if (p_timeout) {
		copyIn(p_timeout, &timeout, sizeof(timeout));
		timeout_us = TS_USEC(timeout); // 等于0表示不等待
	} else {
		// 如果timeout为NULL，表示永久等待
		timeout_us = 1000000ul * 9999999ul;
	}

	u64 start = time_rtc_us();
	if (timeout_us != 0) {
		warn("pselect6: timeout_us = %d\n", timeout_us);
	}

	// debug
	log(LEVEL_GLOBAL, "pselect6: timeout_us = %d\n", timeout_us);
	log(LEVEL_GLOBAL, "readfds: \n");
	FD_SET_FOREACH(fd, &readfds) {
		log(LEVEL_GLOBAL, "%d\n", fd);
	}
	log(LEVEL_GLOBAL, "\n");

	log(LEVEL_GLOBAL, "writefds: \n");
	FD_SET_FOREACH(fd, &writefds) {
		log(LEVEL_GLOBAL, "%d\n", fd);
	}
	log(LEVEL_GLOBAL, "\n");

	log(LEVEL_GLOBAL, "exceptfds: \n");
	FD_SET_FOREACH(fd, &exceptfds) {
		log(LEVEL_GLOBAL, "%d\n", fd);
	}
	log(LEVEL_GLOBAL, "\n");
	// debug end

	while (1) {
		// 创建一套临时的fds数组，用于记录轮询情况
		readfds_cur = readfds;
		writefds_cur = writefds;
		exceptfds_cur = exceptfds;

		int tot = 0; // 就绪的fd数目
		log(FS_GLOBAL, "readfds: \n");
		FD_SET_FOREACH(fd, &readfds_cur) {
			log(FS_GLOBAL, "%d\n", fd);
			r = check_pselect_r(fd);
			if (r < 0) {
				return r;
			} else if (r == 0) {
				FD_CLR(fd, &readfds_cur);
			} else {
				FD_SET(fd, &readfds_cur);
				log(FS_GLOBAL, "Thread %s: read FD_SET %d\n", cpu_this()->cpu_running->td_name, fd);
			}
			tot += r;
		}
		log(FS_GLOBAL, "\n");

		log(FS_GLOBAL, "writefds: \n");
		FD_SET_FOREACH(fd, &writefds_cur) {
			log(FS_GLOBAL, "%d\n", fd);
			r = check_pselect_w(fd);
			if (r < 0) {
				return r;
			} else if (r == 0) {
				FD_CLR(fd, &writefds_cur);
			} else {
				FD_SET(fd, &writefds_cur);
				log(FS_GLOBAL, "Thread %s: write FD_SET %d\n", cpu_this()->cpu_running->td_name, fd);
			}
			tot += r;
		}
		log(FS_GLOBAL, "\n");

		// 暂时省略
		// log(FS_GLOBAL, "exceptfds: \n");
		// FD_SET_FOREACH(fd, &exceptfds_cur) {
		// 	log(FS_GLOBAL, "%d\n", fd);
		// }
		// log(FS_GLOBAL, "\n");

		if (tot > 0) {
			func_ret = tot;
			break;
		} else {
			if (timeout_us >= 10000) {
				// 小睡10ms
				tsleep(&timeout, NULL, "pselect", time_mono_us() + 10000);
			}
			// 否则，循环消耗时间即可
		}

		// 超时退出
		u64 now = time_rtc_us();
		if (timeout_us == 0 || now - start >= timeout_us) {
			func_ret = 0;
			break;
		}
	}

	// 不要返回任何异常情况
	memset(&exceptfds_cur, 0, sizeof(exceptfds_cur));

	// 将轮询状况返回
	if (p_readfds) copyOut(p_readfds, &readfds_cur, sizeof(readfds_cur));
	if (p_writefds) copyOut(p_writefds, &writefds_cur, sizeof(writefds_cur));
	if (p_exceptfds) copyOut(p_exceptfds, &exceptfds_cur, sizeof(exceptfds_cur));
	return func_ret;
}


/**
 * @brief 控制文件描述符的属性
 */
int sys_fcntl(int fd, int cmd, int arg) {
	log(LEVEL_GLOBAL, "fcntl: fd = %d, cmd = %x, arg = %x\n", fd, cmd, arg);

	Fd *kfd;
	int ret = 0;
	if ((kfd = get_kfd_by_fd(fd)) == NULL) {
		return -1;
	}

	mtx_lock_sleep(&kfd->lock);

	switch (cmd) {
	// 目前Linux只规定了 FD_CLOEXEC 用于 fcntl 的 set/get fd
	// FD_CLOEXEC标志对应了fd->flags的__O_CLOEXEC位
	// TODO: 在exec时实现根据fd的FD_CLOEXEC flag来决定是关闭还是保留
	case FCNTL_GETFD:
		ret = (kfd->flags & __O_CLOEXEC) ? FD_CLOEXEC : 0;
		break;
	case FCNTL_SETFD:
		if (arg & FD_CLOEXEC)
			kfd->flags |= __O_CLOEXEC;
		else
			kfd->flags &= ~__O_CLOEXEC;
		break;
	case FCNTL_GET_FILE_STATUS: // 等同于F_GETFL
		ret = kfd->flags;
		break;
	case FCNTL_DUPFD_CLOEXEC:
		// TODO: 实现CLOEXEC标志位
		ret = dup(fd);
		break;
	case FCNTL_SETFL:
		// TODO: 未实现
		kfd->flags = arg; // 主要是O_NONBLOCK
		warn("fcntl: FCNTL_SETFL not implemented\n");
		break;
	default:
		warn("fcntl: unknown cmd %d\n", cmd);
		break;
	}

	mtx_unlock_sleep(&kfd->lock);
	return ret;
}

/**
 * @brief 设置对文件的上次访问和修改时间戳
 * 针对path指定的文件，在times[0]中放置新的上次访问时间，在times[1]中放置新的上次修改时间
 * @param pTime 指向用户态const struct timespec times[2]数组的指针
 * @return 如果文件不存在，返回-1，否则返回0
 */
int sys_utimensat(int dirfd, u64 pathname, u64 pTime, int flags) {
	Dirent *dir, *file;
	char path[MAX_NAME_LEN];
	int ret;

	unwrap(getDirentByFd(dirfd, &dir, NULL));
	if (pathname != 0) {
		copyInStr(pathname, path, MAX_NAME_LEN);
		ret = getFile(dir, path, &file);
	} else {
		ret = getFile(dir, NULL, &file);
	}

	if (ret < 0)
		return ret;
	else if (pTime) {
		struct timespec times[2];
		copyIn(pTime, (void *)times, sizeof(times));
		file_set_timestamp(file, ACCESS_TIME, &times[0]);
		file_set_timestamp(file, MODIFY_TIME, &times[1]);
		file_close(file);
		return 0;
	} else {
		// pTime == 0
		file_update_timestamp(file, ACCESS_TIME);
		file_update_timestamp(file, MODIFY_TIME);
		file_close(file);
		return 0;
	}
}

int sys_renameat2(int olddirfd, u64 oldpath, int newdirfd, u64 newpath, unsigned int flags) {
	Dirent *olddir, *newdir;
	char oldpathStr[MAX_NAME_LEN], newpathStr[MAX_NAME_LEN];
	unwrap(getDirentByFd(olddirfd, &olddir, NULL));
	unwrap(getDirentByFd(newdirfd, &newdir, NULL));
	copyInStr(oldpath, oldpathStr, MAX_NAME_LEN);
	copyInStr(newpath, newpathStr, MAX_NAME_LEN);

	return renameat2(olddir, oldpathStr, newdir, newpathStr, flags);
}

// position read，从特定位置开始读取文件
size_t sys_pread64(int fd, u64 buf, size_t count, off_t offset) {
	return pread64(fd, buf, count, offset);
}

size_t sys_pwrite64(int fd, u64 buf, size_t count, off_t offset) {
	return pwrite64(fd, buf, count, offset);
}

#define MSDOS_SUPER_MAGIC 0x4d44 /* MD */

int sys_statfs(u64 ppath, struct statfs *buf) {
	Dirent *file;
	char path[MAX_NAME_LEN];
	copyInStr(ppath, path, MAX_NAME_LEN);
	struct statfs statfs;

	memset(&statfs, 0, sizeof(statfs));
	int ret = getFile(NULL, path, &file);
	if (ret < 0) {
		return ret;
	} else {
		FileSystem *fs = file->file_system;

		// 分配出的扇区数
		extern u64 alloced_clus;
		// 使用中的dirent数
		extern u64 used_dirents;

		assert(fs != NULL);
		statfs.f_type = MSDOS_SUPER_MAGIC;
		statfs.f_bsize = fs->superBlock.bytes_per_clus;
		statfs.f_blocks = fs->superBlock.bpb.tot_sec / fs->superBlock.bpb.sec_per_clus;
		statfs.f_bfree = MAX(statfs.f_blocks / 2 - alloced_clus, 0);		 // TODO: 空闲块数（有意偏少）
		statfs.f_bavail = statfs.f_bfree;		 // 空闲块数
		statfs.f_files = MAX_DIRENT;			 // 虚构的文件数
		statfs.f_ffree = MAX(MAX_DIRENT - used_dirents, 0);				 // 虚构的空闲file node数（有意偏少）
		statfs.f_fsid.val[0] = statfs.f_fsid.val[1] = 0; // fsid，一般不用
		statfs.f_namelen = MAX_NAME_LEN;
		statfs.f_frsize = 0; // unknown
		statfs.f_flags = 0;  // 无需处理

		copyOut((u64)buf, &statfs, sizeof(statfs));
		file_close(file);
		return 0;
	}
}

// 改变文件的大小
int sys_ftruncate(int fd, off_t length) {
	Dirent *file;
	unwrap(getDirentByFd(fd, &file, NULL));

	extern mutex_t mtx_file;
	mtx_lock_sleep(&mtx_file);

	if (length <= file->file_size) {
		file_shrink(file, length);
	} else {
		file_extend(file, length);
	}

	mtx_unlock_sleep(&mtx_file);
	return 0;
}

int sys_readlinkat(int dirfd, u64 pathname, u64 buf, size_t bufsiz) {
	Dirent *dir, *file;
	char path[MAX_NAME_LEN];
	int ret;

	unwrap(getDirentByFd(dirfd, &dir, NULL));
	copyInStr(pathname, path, MAX_NAME_LEN);
	unwrap(getFile(dir, path, &file));

	log(LEVEL_GLOBAL, "readlinkat: %s\n", path);

	// 不是链接文件
	if (!IS_LINK(&file->raw_dirent)) {
		file_close(file);
		warn("readlinkat: %s is not a link\n", path);
		return -EINVAL;
	}

	size_t len = MIN(file->file_size, bufsiz - 1);
	ret = file_read(file, 1, buf, 0, len);
	((char *)buf)[len] = '\0';
	file_close(file);
	return ret;
}

int sys_sync() {
	bufSync();
	return 0;
}

int sys_syncfs(int fd) {
	return 0;
}


