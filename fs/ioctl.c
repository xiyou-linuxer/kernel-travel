#include <fs/vfs.h>
#include <fs/fs.h>
#include <fs/fd.h>
#include <xkernel/string.h>
#include <asm-generic/ioctl.h>
int ioctl_fsfreeze(struct fd *filp)
{
	filp->dirent->file_system->op->file_write = NULL;
	return 0;
}

int ioctl_fsthaw(struct fd * filp)
{
	filp->dirent->file_system->op->fsthaw(); 
	return 0;
}

int do_vfs_ioctl(struct fd *filp, unsigned int fd, unsigned int cmd, unsigned long arg)
{
	int error = 0;
	// 将用户空间的参数转换为 int 类型的指针
	int *argp = (int *)arg;

	// 只实现了部分操作
	switch (cmd) {
		case FIOCLEX:
			// 设置文件描述符为在关闭时自动执行文件描述符关闭的操作
			//set_close_on_exec(fd, 1);
			break;

		case FIONCLEX:
			// 清除文件描述符的自动关闭标志
			//set_close_on_exec(fd, 0);
			break;

		case FIONBIO:
			// 设置或取消非阻塞模式
			//error = ioctl_fionbio(filp, argp);
			break;

		case FIOASYNC:
			// 设置或取消异步通知模式
			//error = ioctl_fioasync(fd, filp, argp);
			break;

		case FIOQSIZE:
			// 获取文件的大小，并返回给用户空间
			if (filp->dirent->type == DIRENT_DIR ||
				filp->dirent->type == DIRENT_FILE ||
				filp->dirent->type == DIRENT_SOFTLINK) {
				long res = filp->dirent->file_size;
				// 将文件大小复制到用户空间的 `arg` 指针指向的位置
				memcpy(arg, &res, sizeof(res));
			} else {
				// 如果文件类型不支持 FIOQSIZE，返回 -ENOTTY（不适用的控制操作）
				error = -1;
			}
			break;

		case FIFREEZE:
			// 冻结文件系统，使其进入只读模式
			error = ioctl_fsfreeze(filp);
			break;

		case FITHAW:
			// 解除冻结的文件系统，使其恢复读写模式
			error = ioctl_fsthaw(filp);
			break;

		/*case FS_IOC_FIEMAP:
			// 获取文件的空间映射信息
			return ioctl_fiemap(filp, arg);*/

		case FIGETBSZ:
		{
			filp->dirent->file_system->superBlock.bytes_per_clus;
			int *p = (int *)arg;
			
		}

		/*default:
			// 对于未处理的命令，尝试通过文件系统或虚拟文件系统进行处理
			if (filp->dirent->type == DIRENT_FILE)
				// 对于常规文件，调用 file_ioctl 处理命令
				error = file_ioctl(filp, cmd, arg);
			else
				// 对于其他类型的文件，调用 vfs_ioctl 处理命令
				error = vfs_ioctl(filp, cmd, arg);
			break;*/
	}
	// 返回处理的错误码
	return error;
}
