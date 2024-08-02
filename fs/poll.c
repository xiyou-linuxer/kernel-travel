#include <asm/timer.h>
#include <fs/fs.h>
#include <fs/fd.h>
#include <xkernel/ioqueue.h>
#include <xkernel/console.h>
#include <xkernel/thread.h>
#include <sync.h>
#include <fs/poll.h>
#include <xkernel/stdio.h>
int check_pselect_r(int fd) {
	int _fd = fd_local2global(fd);
	struct fd *kfd = &file_table[_fd];
	int ret;
	if (kfd == NULL) {
		return -1;
	}
	// pipe,console
	if (kfd->type == dev_pipe) {
		ret = pipe_check_read(&kfd->pipe);
	} else if (kfd->type == dev_console) {
		console_acquire();
		ret = console_check_read();
	} else {
		ret = 1;
	}
	return ret;
}

/**
 * @brief 检查fd是否可以写入。可以返回1，不可以返回0，失败返回负数
 */
int check_pselect_w(int fd) {
	int _fd = fd_local2global(fd);
	struct fd *kfd = &file_table[_fd];
	int ret;
	if (kfd == NULL) {
		printk("pselect6: fd %d not found\n", fd);
		return -1;
	}
	// pipe,console
	if (kfd->type == dev_pipe) {
		ret = pipe_check_write(&kfd->pipe);
	} else if (kfd->type == dev_console) {
		ret = 1;
	} else {
		ret = 1;
	}
	return ret;
}

int sys_ppoll(unsigned long p_fds, int nfds, unsigned long tmo_p, unsigned long sigmask) {
	struct pollfd poll_fd;
	struct timespec tmo;
	int ret = 0;
	sys_sleep(&tmo,0);
	while (1) {
		ret = 0;
		for (int i = 0; i < nfds; i++) {
			u64 cur_fds = p_fds + i * sizeof(poll_fd);
			memcpy(&poll_fd, cur_fds, sizeof(poll_fd));
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
			memcpy(cur_fds, &poll_fd, sizeof(poll_fd));

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

