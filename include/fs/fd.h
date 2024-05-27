#ifndef __FS_FD_H
#define __FS_FD_H

#include <xkernel/types.h>
#include <fs/fs.h>
#include <sync.h>
#include <xkernel/ioqueue.h>
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
	struct ioqueue pipe;
	int type;
	unsigned int offset;
	unsigned int flags;
	struct kstat stat;
	unsigned int refcnt; // 引用计数
} fd;

struct statx_timestamp {
    __s64 tv_sec;   // 秒
    __u32 tv_nsec;  // 纳秒
    __s32 __reserved;
};

struct statx {
    __u32 stx_mask;        // 表示哪些字段有效的掩码
    __u32 stx_blksize;     // 优化I/O的块大小
    __u64 stx_attributes;  // 文件的属性
    __u32 stx_nlink;       // 硬链接数
    __u32 stx_uid;         // 所有者用户ID
    __u32 stx_gid;         // 所有者组ID
    __u16 stx_mode;        // 文件类型和模式
    __u16 __spare0[1];
    __u64 stx_ino;         // inode编号
    __u64 stx_size;        // 文件大小
    __u64 stx_blocks;      // 分配的块数
    __u64 stx_attributes_mask; // 表示哪些属性有效的掩码
    struct statx_timestamp stx_atime; // 上次访问时间
    struct statx_timestamp stx_btime; // 创建时间
    struct statx_timestamp stx_ctime; // 上次状态更改时间
    struct statx_timestamp stx_mtime; // 上次修改时间
    __u32 stx_rdev_major;  // 设备的主设备号
    __u32 stx_rdev_minor;  // 设备的次设备号
    __u32 stx_dev_major;   // 文件所在设备的主设备号
    __u32 stx_dev_minor;   // 文件所在设备的次设备号
    __u64 __spare2[14];    // 备用字段
};

struct linux_dirent64 {
    unsigned long d_ino;              // 索引结点号
    long d_off;	// 到下一个dirent的偏移
    unsigned short d_reclen;	// 当前dirent的长度
    unsigned char d_type;	// 文件类型
    char d_name[MAX_NAME_LEN];	//文件名
};

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
int filename2path(Dirent* file, char* newpath);
void fd_mapping(int fd, int start_page, int end_page, unsigned long* v_addr);
bool is_pipe(uint32_t local_fd);
uint32_t pipe_read(int32_t fd, void* buf, uint32_t count);
uint32_t pipe_write(int32_t fd, const void* buf, uint32_t count);
void sys_fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd);
#endif