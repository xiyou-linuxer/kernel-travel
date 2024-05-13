#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <linux/types.h>
#include <fs/fs.h>
#include <debug.h>
#include <linux/list.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fd.h>
#include <linux/sched.h>
struct fd file_table[MAX_FILE_OPEN];//全局文件打开数组

/* 从文件表file_table中获取一个空闲位,成功返回下标,失败返回-1 */
int32_t get_free_slot_in_global(void)
{
	uint32_t fd_idx = 3;
	while (fd_idx < MAX_FILE_OPEN)
	{
		if (file_table[fd_idx].dirent == NULL)
		{
			break;
		}
		fd_idx++;
	}
	if (fd_idx == MAX_FILE_OPEN)
	{
		printk("exceed max open files\n");
		return -1;
	}
	return fd_idx;
}

/* 将全局描述符下标安装到进程或线程自己的文件描述符数组fd_table中,
 * 成功返回下标,失败返回-1 */
int32_t pcb_fd_install(int32_t globa_fd_idx)
{
	struct task_struct *cur = running_thread();
	uint8_t local_fd_idx = 3; // 跨过stdin,stdout,stderr
	while (local_fd_idx < MAX_FILES_OPEN_PER_PROC)
	{
		if (cur->fd_table[local_fd_idx] == -1)
		{ // -1表示free_slot,可用
			cur->fd_table[local_fd_idx] = globa_fd_idx;
			file_table[globa_fd_idx].refcnt++;
			break;
		}
		local_fd_idx++;
	}
	if (local_fd_idx == MAX_FILES_OPEN_PER_PROC)
	{
		printk("exceed max open files_per_proc\n");
		return -1;
	}
	return local_fd_idx;
}

/* 将文件描述符转化为文件表的下标 */
uint32_t fd_local2global(uint32_t local_fd)
{
    struct task_struct *cur = running_thread();
    int32_t global_fd = cur->fd_table[local_fd];
    ASSERT(global_fd >= 0 && global_fd < MAX_FILE_OPEN);
    return (uint32_t)global_fd;
}

int file_open(Dirent* file, int flag, mode_t mode)
{
	/*若文件在打开的列表则直接将引用计数加一后返回*/
	for (int i = 0; i < MAX_FILE_OPEN; i++)
	{
		if (file_table[i].dirent == file)
		{
			file_table[i].refcnt += 1;
			return pcb_fd_install(i);
		}
	}
	
	int fd_idx = get_free_slot_in_global();
	if (fd_idx == -1)
	{
		printk("exceed max open files\n");
		return -1;
	}
	file_table[fd_idx].dirent->refcnt += 1;
	file_table[fd_idx].dirent = file;
	file_table[fd_idx].offset =0; // 每次打开文件,要将offset还原为0,即让文件内的指针指向开头
	file_table[fd_idx].flags = flag;//文件打开的标志位
	file_table[fd_idx].stat.st_mode = mode;
	file_table[fd_idx].type = dev_file;
	file_table[fd_idx].refcnt = 1;
	lock_init(&file_table[fd_idx].lock);
	return pcb_fd_install(fd_idx);
}

int file_create(struct Dirent *baseDir, char *path, int flag,mode_t mode)
{
	Dirent *file;
	createFile(baseDir, path, &file);//创建文件
	int fd_idx = get_free_slot_in_global();
	if (fd_idx == -1)
	{
		printk("exceed max open files\n");
	}
	file_table[fd_idx].dirent = file;
	file_table[fd_idx].offset =0;
	file_table[fd_idx].flags = flag;
	file_table[fd_idx].stat.st_mode = mode;
	file_table[fd_idx].type = dev_file;
	lock_init(&file_table[fd_idx].lock);//初始化文件锁
	return pcb_fd_install(fd_idx);
}

int file_close(struct fd *_fd)
{
	if (_fd->dirent == NULL)
	{
		return -1;
	}
	_fd->refcnt -= 1;
	if (_fd->refcnt == 0)//fd的引用计数为0则释放这个fd
	{
		_fd->dirent= NULL;
		_fd->offset = 0;
		_fd->offset = -1;
		_fd->type = -1;
	}
	return 0;
}