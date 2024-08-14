#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fd.h>
#include <fs/fs.h>
#include <fs/filepnt.h>
#include <fs/path.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <xkernel/sched.h>
#include <xkernel/thread.h>
#include <xkernel/list.h>
#include <xkernel/ioqueue.h>
#include <trap/irq.h>
#include <debug.h>

struct fd file_table[MAX_FILE_OPEN];//全局文件打开数组
int pipe_table[10][2];
/* 从文件表file_table中获取一个空闲位,成功返回下标,失败返回-1 */
int32_t get_free_slot_in_global(void)
{
	uint32_t fd_idx = 3;
	while (fd_idx < MAX_FILE_OPEN)
	{
		if (file_table[fd_idx].type == dev_pipe) {
			fd_idx++;
			continue;
		}
		if (file_table[fd_idx].dirent == NULL)
		{
			break;
		}
		fd_idx++;
	}
	if (fd_idx == MAX_FILE_OPEN)
	{
		//printk("exceed max open files\n");
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
		return 0;
	}
	return local_fd_idx;
}

/* 将文件描述符转化为文件表的下标 */
uint32_t fd_local2global(uint32_t local_fd)
{
    struct task_struct *cur = running_thread();
    int global_fd = cur->fd_table[local_fd];
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
	file_table[fd_idx].dirent = file;
	file_table[fd_idx].dirent->refcnt += 1;
	file_table[fd_idx].offset = 0; // 每次打开文件,要将offset还原为0,即让文件内的指针指向开头
	file_table[fd_idx].flags = flag;//文件打开的标志位
	file_table[fd_idx].stat.st_mode = mode;
	file_table[fd_idx].type = dev_file;
	file_table[fd_idx].refcnt = 1;
	lock_init(&file_table[fd_idx].lock);
	if (file->file_system->op->file_init != NULL)
	{
		file->file_system->op->file_init(file);//初始化文件
	}
	return pcb_fd_install(fd_idx);
}

int file_create(struct Dirent *baseDir, char *path, int flag,mode_t mode)
{
	Dirent *file;
	baseDir->file_system->op->file_create(baseDir, path, &file);//创建文件
	//createFile(baseDir, path, &file);
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
		_fd->dirent->refcnt -=1;
		_fd->dirent= NULL;
		_fd->offset = 0;
		_fd->offset = -1;
		_fd->type = -1;
	}
	return 0;
}

bool is_pipe(uint32_t local_fd)
{
	uint32_t global_fd = fd_local2global(local_fd);
	//printk("type:%d", file_table[global_fd].type);
	return file_table[global_fd].type == dev_pipe;
}

/* 从管道中读数据 */
uint32_t pipe_read(int32_t fd, void *buf, uint32_t count)
{
    char *buffer = buf;
    uint32_t bytes_read = 0;
    uint32_t global_fd = fd_local2global(fd);

    /* 获取管道的环形缓冲区 */
    struct ioqueue *ioq = &file_table[global_fd].pipe;

    /* 选择较小的数据读取量,避免阻塞 */
    //uint32_t ioq_len = ioq_length(ioq);
    //uint32_t size = ioq_len > count ? count : ioq_len;
    while (bytes_read < count)
    {
    	*buffer = ioq_getchar(ioq);
		if (*buffer == '@')
		{
			return -1;
		}
        bytes_read++;
        buffer++;
    }
    return bytes_read;
}

/* 往管道中写数据 */
uint32_t pipe_write(int32_t fd, const void *buf, uint32_t count)
{
    uint32_t bytes_write = 0;
    uint32_t global_fd = fd_local2global(fd);
    struct ioqueue *ioq = &file_table[global_fd].pipe;

    /* 选择较小的数据写入量,避免阻塞 */
    uint32_t ioq_left = bufsize - ioq_length(ioq) -1;
    uint32_t size = ioq_left > count ? count : ioq_left;

    const char *buffer = buf;
    while (bytes_write < size)
    {
		if (ioq->flag==0)
		{
			return 0;
		}
        ioq_putchar(ioq, *buffer);
        bytes_write++;
        buffer++;
    }
    return bytes_write;
}
