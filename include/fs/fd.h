#ifndef __FS_FD_H
#define __FS_FD_H

#include <xkernel/types.h>
#include <fs/fs.h>
#include <sync.h>

#define MAX_FILE_OPEN 64 //全局最大文件打开数量

/* 打开文件的选项 */
enum oflags {
	O_DIRECTORY,
	O_RDONLY,	// 只读
	O_WRONLY,	// 只写
	O_RDWR,		// 读写
	O_CREATE = 4 // 创建
};

enum whence
{
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
};

struct kstat {
	unsigned long st_dev;
	unsigned long st_ino;
	unsigned int st_mode;
	unsigned int st_nlink;
	unsigned int st_uid;
	unsigned int st_gid;
	unsigned long st_rdev;
	unsigned long __pad;
	int st_size;
	unsigned int st_blksize;
	int __pad2;
	unsigned long st_blocks;
	long st_atime_sec;
	long st_atime_nsec;
	long st_mtime_sec;
	long st_mtime_nsec;
	long st_ctime_sec;
	long st_ctime_nsec;
	unsigned __unused[2];
};

enum std_fd {
   STDIN,   // 0 标准输入
   STDOUT,  // 1 标准输出
   STDERR   // 2 标准错误
};

typedef struct fd {
	// 保证每个fd的读写不并发
	struct lock lock;
	struct Dirent *dirent;
	//struct Pipe *pipe;
	int type;
	unsigned int offset;
	unsigned int flags;
	struct kstat stat;
	unsigned int refcnt; // 引用计数
} fd;

#define dev_file 1
#define dev_pipe 2
#define dev_console 3

extern struct fd file_table[MAX_FILE_OPEN];//全局文件打开数组

int32_t get_free_slot_in_global(void);//获取全局描述符
int32_t pcb_fd_install(int32_t globa_fd_idx);//将全局描述符下载到自己的线程中
uint32_t fd_local2global(uint32_t local_fd);
int file_open(Dirent* file, int flag, mode_t mode);  // 打开文件
int file_create(struct Dirent* baseDir, char* path, int flag, mode_t mode);//创建文件
int file_close(struct fd *_fd);
int rmfile(struct Dirent* file);
#endif