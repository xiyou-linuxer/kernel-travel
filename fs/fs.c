#include <fs/fs.h>
#include <fs/buf.h>
#include <linux/stdio.h>
#include <debug.h>
#include <linux/string.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
static struct FileSystem fs[MAX_FS_COUNT];

static Buffer *getBlock(FileSystem *fs, u64 blockNum, bool is_read) 
{
	ASSERT(fs != NULL);

	if (fs->image == NULL) {
		// 是挂载了根设备，直接读取块缓存层的数据即可
		return bufRead(fs->deviceNumber, blockNum, is_read);
	} else {
		// 处理挂载了文件的情况
		Dirent *img = fs->image;
		struct FileSystem *parentFs = fs->image->file_system;//找到文件系统挂载的父节点
		int blockNo = fileBlockNo(parentFs, img->first_clus, blockNum);
		return bufRead(parentFs->deviceNumber, blockNo, is_read);
	}
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
	PANIC();
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


/**
 * @brief 初始化分区信息，填写文件系统结构体里面的超级块
 */
int partition_format(FileSystem *fs) {
	printk("Fat32 FileSystem Init Start\n");
	// 读取 BPB
	ASSERT(fs != NULL);
	ASSERT(fs->get != NULL);

	Buffer *buf = fs->get(fs, 0, true);
	if (buf == NULL) {
		printk("buf == NULL\n");
		return -E_DEV_ERROR;
	}
	/*for (int i = 0; i < 512; i++)
	{
		printk("%x",buf->data->data[i]);
	}*/
	
	printk("cluster DEV is ok!\n");

	// 从 BPB 中读取信息
	FAT32BootParamBlock *bpb = (FAT32BootParamBlock *)(buf->data->data);

	if (bpb == NULL || strncmp((char *)bpb->BS_FilSysType, "FAT32", 5)) {
		printk("Not FAT32 File System\n");
		return -E_UNKNOWN_FS;
	}
	fs->superBlock.bpb.bytes_per_sec = bpb->BPB_BytsPerSec;
	fs->superBlock.bpb.sec_per_clus = bpb->BPB_SecPerClus;
	fs->superBlock.bpb.rsvd_sec_cnt = bpb->BPB_RsvdSecCnt;
	fs->superBlock.bpb.fat_cnt = bpb->BPB_NumFATs;
	fs->superBlock.bpb.hidd_sec = bpb->BPB_HiddSec;
	fs->superBlock.bpb.tot_sec = bpb->BPB_TotSec32;
	fs->superBlock.bpb.fat_sz = bpb->BPB_FATSz32;
	fs->superBlock.bpb.root_clus = bpb->BPB_RootClus;

	printk("cluster Get superblock!\n");

	// 填写超级块
	fs->superBlock.first_data_sec = bpb->BPB_RsvdSecCnt + bpb->BPB_NumFATs * bpb->BPB_FATSz32;
	fs->superBlock.data_sec_cnt = bpb->BPB_TotSec32 - fs->superBlock.first_data_sec;
	fs->superBlock.data_clus_cnt = fs->superBlock.data_sec_cnt / bpb->BPB_SecPerClus;
	fs->superBlock.bytes_per_clus = bpb->BPB_SecPerClus * bpb->BPB_BytsPerSec;
	if (BUF_SIZE != fs->superBlock.bpb.bytes_per_sec) {
		printk("BUF_SIZE != fs->superBlock.bpb.bytes_per_sec\n");
		return -E_DEV_ERROR;
	}

	printk("cluster ok!\n");

	// 释放缓冲区
	bufRelease(buf);

	printk("buf release!\n");
	return 0;
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

/*文件系统初始化*/
void fs_init(void)
{
	printk("fs_init start\n");
	bufInit();			//初始化buf
	//bufTest(0);
	dirent_init();
	init_root_fs();		//初始化根文件系统
	printk("init_root_fs down\n");
	//int i = 0;
	fat32Test() ;
	printk("fs_init down\n");
}