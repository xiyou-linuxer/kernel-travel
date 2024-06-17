#ifndef _FS_H
#define _FS_H

#include <xkernel/list.h>
#include <xkernel/types.h>
#include <asm-generic/int-ll64.h>
#include <asm/page.h>
#include <fs/fat32.h>
#include <fs/ext4_types.h>
#include <fs/file_time.h>

#define MAX_FS_COUNT 16 //最多可挂载的文件系统数量
#define MAX_NAME_LEN 128 //文件名的最大字数限制
#define PAGE_NCLUSNO (PAGE_SIZE / sizeof(unsigned int))// 一页能容纳的u32簇号个数
#define NDIRENT_SECPOINTER 5
#define MAX_DIRENT 160000

typedef struct FileSystem FileSystem;
typedef struct Dirent Dirent;
typedef struct SuperBlock SuperBlock;

// 对应目录、文件、设备
typedef enum dirent_type { DIRENT_DIR, DIRENT_FILE, DIRENT_CHARDEV, DIRENT_BLKDEV , DIRENT_UNKNOWN} dirent_type_t;

/*文件系统的操作函数*/
struct fs_operation{
	/*文件系统的初始化*/
	void (*fs_init_ptr)(FileSystem*);//指向文件系统初始化的指针
	/*文件操作*/
	//创建文件
	//打开文件
	//读文件
	//写文件
	/*目录操作*/
	//创建目录
};

typedef struct SuperBlock {
	u32 first_data_sec;
	u32 data_sec_cnt;
	u32 data_clus_cnt;
	u32 bytes_per_clus;
	union 
	{
		struct bpb bpb;
		struct ext4_sblock ext4_sblock;
	};
}SuperBlock;

typedef struct FileSystem {
	bool valid; // 是否有效
	char name[8];
	struct SuperBlock superBlock;					// 超级块
	struct Dirent *root;						 	// root项
	struct Dirent *mountPoint;				// 挂载点
	int deviceNumber;						// 对应真实设备的编号
	struct Buffer *(*get)(struct FileSystem *fs, u64 blockNum, bool is_read); // 获取fs超级快的方式
	struct FileSystem* next;
	struct fs_operation* op;//文件系统的操作函数
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

typedef struct Dirent {
	union 
	{
		FAT32Directory raw_dirent; // 原生的dirent项
	};
	char name[MAX_NAME_LEN];
	// 文件系统相关属性
	FileSystem *file_system; 	// dir所在的文件系统
	unsigned int first_clus;	// 第一个簇的簇号（如果为0，表示文件尚未分配簇）
	unsigned int file_size;		// 文件大小
	/* for OS */
	struct vfsmount *head;		//挂载在当前目录下的
	DirentPointer pointer;
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
	struct list_elem dirent_tag; //链表节点，用于父目录记录
	// 父亲Dirent
	struct Dirent *parent_dirent; // 即使是mount的目录，也指向其上一级目录。如果该字段为NULL，表示为总的根目录
	u32 mode;
	// 各种计数
	unsigned short linkcnt; // 链接计数
	unsigned short refcnt;  // 引用计数
}Dirent;

extern struct lock mtx_file;

#define MAX_LONGENT 8

typedef struct longEntSet {
	FAT32LongDirectory *longEnt[MAX_LONGENT];
	int cnt;
} longEntSet;


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

#define MIN(_a, _b)                                                                                \
	({                                                                                         \
		typeof(_a) __a = (_a);                                                             \
		typeof(_b) __b = (_b);                                                             \
		__a <= __b ? __a : __b;                                                            \
	})

// 舍入到更大的页对齐地址
#define PGROUNDUP(a) (((a) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PAGE_SIZE - 1))
#define ROUNDUP(a, x) (((a) + (x)-1) & ~((x)-1))

/*表示文件系统中的最大文件大小*/
#define MAX_LFS_FILESIZE 	0x7fffffffffffffffUL

extern FileSystem* fatFs;

void vfs_init(void);//初始化VFS
void fs_init(void);//在init进程中进行
typedef int (*findfs_callback_t)(FileSystem *fs, void *data);
void allocFs(struct FileSystem **pFs);
void deAllocFs(struct FileSystem *fs);
int partition_format(FileSystem* fs);//初始化文件系统分区
FileSystem *find_fs_by(findfs_callback_t findfs, void *data);
void fat32_init(struct FileSystem* fs) ;
int is_directory(FAT32Directory* f);
void fat32Test(void);
#endif