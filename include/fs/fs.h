#ifndef _FS_H
#define _FS_H

#include <linux/list.h>
#include <linux/types.h>
#include <asm-generic/int-ll64.h>
#include <fs/fat32.h>
#include <asm/page.h>
#include <fs/file_time.h>

#define MAX_FS_COUNT 4 //最多可挂载的文件系统数量
#define MAX_NAME_LEN 128 //文件名的最大字数限制
#define PAGE_NCLUSNO (PAGE_SIZE / sizeof(u32))// 一页能容纳的u32簇号个数
#define NDIRENT_SECPOINTER 5

struct bpb{
	u16 bytes_per_sec;
	u8 sec_per_clus;
	u16 rsvd_sec_cnt;
	u8 fat_cnt;   /* count of FAT regions */
	u32 hidd_sec; /* count of hidden sectors */
	u32 tot_sec;  /* total count of sectors including all regions */
	u32 fat_sz;   /* count of sectors for a FAT region */
	u32 root_clus;
};

typedef struct SuperBlock {
	u32 first_data_sec;
	u32 data_sec_cnt;
	u32 data_clus_cnt;
	u32 bytes_per_clus;
	struct bpb bpb;
}SuperBlock;

typedef struct FileSystem {
	bool valid; // 是否有效
	char name[8];
	struct SuperBlock superBlock;					// 超级块
	struct Dirent *root;						 	// root项
	struct Dirent *image;					// mount对应的文件描述符
	struct Dirent *mountPoint;				// 挂载点
	int deviceNumber;						// 对应真实设备的编号
	struct Buffer *(*get)(struct FileSystem *fs, u64 blockNum, bool is_read); // 读取FS的一个Buffer
	// 强制规定：传入的fs即为本身的fs
	// 稍后用read返回的这个Buffer指针进行写入和释放动作
	// 我们默认所有文件系统（不管是挂载的，还是从virtio读取的），都需要经过缓存层
}FileSystem;

// 二级指针
struct TwicePointer {
	u32 cluster[PAGE_NCLUSNO];
};

// 三级指针
struct ThirdPointer {
	struct TwicePointer *ptr[PAGE_SIZE / sizeof(struct TwicePointer *)];
};

// 指向簇列表的指针
typedef struct DirentPointer {
	// 一级指针
	// u32 first[10];
	// 简化：只使用二级指针和三级指针
	struct TwicePointer *second[NDIRENT_SECPOINTER];
	struct ThirdPointer *third;
	u16 valid;
} DirentPointer;

#define MAX_LONGENT 8

typedef struct longEntSet {
	FAT32LongDirectory *longEnt[MAX_LONGENT];
	int cnt;
} longEntSet;

typedef struct Dirent {
	FAT32Directory raw_dirent; // 原生的dirent项
	char name[MAX_NAME_LEN];

	// 文件系统相关属性
	FileSystem *file_system; // 所在的文件系统
	unsigned int first_clus;		 // 第一个簇的簇号（如果为0，表示文件尚未分配簇）
	unsigned int file_size;		 // 文件大小

	/* for OS */
	// 操作系统相关的数据结构
	// 仅用于是挂载点的目录，指向该挂载点所对应的文件系统。用于区分mount目录和非mount目录
	FileSystem *head;

	DirentPointer pointer;

	// [暂不用] 标记此Dirent节点是否已扩展子节点，用于弹性伸缩Dirent缓存，不过一般设置此字段为1
	// 我们会在初始化时扫描所有文件，并构建Dirent
	// u16 is_extend;

	// 在上一个目录项中的内容偏移，用于写回
	unsigned int parent_dir_off;

	// 标记是文件、目录还是设备文件（仅在文件系统中出现，不出现在磁盘中）
	unsigned short type;

	unsigned short is_rm;

	// 文件的时间戳
	struct file_time time;

	// 设备结构体，可以通过该结构体完成对文件的读写
	struct FileDev *dev;

	// 子Dirent列表
	struct list child_list;
	struct list_elem dirent_tag;//链表节点，用于父目录记录
	// 用于空闲链表和父子连接中的链接，因为一个Dirent不是在空闲链表中就是在树上
	//LIST_ENTRY(Dirent) dirent_link;

	// 父亲Dirent
	struct Dirent *parent_dirent; // 即使是mount的目录，也指向其上一级目录。如果该字段为NULL，表示为总的根目录

	unsigned int mode;

	// 各种计数
	unsigned short linkcnt; // 链接计数
	unsigned short refcnt;  // 引用计数

	//struct holder_info holders[DIRENT_HOLDER_CNT];
	//int holder_cnt;
}Dirent;

enum fs_result {
	E_NO_MAP,			//无法创建映射，用于mmap
	E_BAD_ELF, 			//ELF 文件损坏
	E_UNKNOWN_FS,		//未知的文件系统
	E_DEV_ERROR,		//设备错误
	E_NOT_FOUND,		//未找到指定的项目
	E_BAD_PATH,			//无效的路径
	E_FILE_EXISTS,		//文件已存在
	E_EXCEED_FILE,		//文件大小超过限制
};

enum dirent_type
{ 
	DIRENT_DIR, 
	DIRENT_FILE, 
	DIRENT_CHARDEV, 
	DIRENT_BLKDEV 
};

struct kstat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	unsigned long __pad;
	off_t st_size;
	blksize_t st_blksize;
	int __pad2;
	blkcnt_t st_blocks;
	long st_atime_sec;
	long st_atime_nsec;
	long st_mtime_sec;
	long st_mtime_nsec;
	long st_ctime_sec;
	long st_ctime_nsec;
	unsigned __unused[2];
};

extern FileSystem* fatFs;

typedef int (*findfs_callback_t)(FileSystem *fs, void *data);
void allocFs(struct FileSystem **pFs);
void deAllocFs(struct FileSystem *fs);
int partition_format(FileSystem* fs);//初始化文件系统分区
void fat32_init(struct FileSystem* fs) ;
void fs_init(void);
#endif