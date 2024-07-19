#include <fs/fs.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/ramfs.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <debug.h>
static struct FileSystem fs[MAX_FS_COUNT];

static Buffer *getBlock(FileSystem *fs, u64 blockNum, bool is_read) 
{
	ASSERT(fs != NULL);
	// 是挂载了根设备，直接读取块缓存层的数据即可
	return bufRead(fs->deviceNumber, blockNum, is_read);
}

/**
 * @brief 分配一个文件系统结构体
 */
void allocFs(FileSystem **pFs) 
{
	for (int i = 0; i < MAX_FS_COUNT; i++) {
		if (fs[i].valid == 0) {
			*pFs = &fs[i];
			memset(&fs[i], 0, sizeof(FileSystem));
			fs[i].valid = 1;
			fs[i].get = getBlock;
			return;
		}
	}
}

/**
 * @brief 释放一个文件系统结构体
 */
void deAllocFs(struct FileSystem *fs) {
	fs->valid = 0;
	memset(fs, 0, sizeof(struct FileSystem));
}


FileSystem *find_fs_by(findfs_callback_t findfs, void *data) {
	for (int i = 0; i < MAX_FS_COUNT; i++) {
		if (findfs(&fs[i], data)) {
			return &fs[i];
		}
	}
	return NULL;
}


int find_fs_of_dir(FileSystem *fs, void *data) {
	Dirent *dir = (Dirent *)data;
	if (fs->mountPoint == NULL) {
		return 0;
	} else {
		return fs->mountPoint->first_clus == dir->first_clus;
	}
}

int get_entry_count_by_name(char *name) {
	int len = (strlen(name) + 1);

	if (len > 11) {
		int cnt = 1; // 包括短文件名项
		if (len % BYTES_LONGENT == 0) {
			cnt += len / BYTES_LONGENT;
		} else {
			cnt += len / BYTES_LONGENT + 1;
		}
		return cnt;
	} else {
		// 自己一个，自己的长目录项一个
		return 2;
	}
}

void vfs_init(void)
{
	printk("fs_init start\n");
	bufInit();			//初始化buf
	dirent_init();		//初始化目录列表
	init_rootfs();		//初始化根文件系统
}

/*加载磁盘文件系统并完成rootfs的迁移*/
void fs_init(void)
{
	printk("fs_init start\n");
	init_root_fs();	//将根目录从虚拟文件系统rootfs转移到磁盘文件系统中fat32/ext4
	printk("ext4_fs init down\n");
	test_fs_all();

	printk("fs_init down\n");
}
