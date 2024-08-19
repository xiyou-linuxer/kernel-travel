#include <fs/fs.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <debug.h>
#include <xkernel/string.h>
#include <xkernel/printk.h>
#include <fs/fat32.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/filepnt.h>
#include <fs/cluster.h>
#include <fs/mount.h>
#include <fs/buf.h>
#include <sync.h>

FileSystem *fatFs;
struct lock mtx_file;

/*注册fat32文件系统的操作函数*/
static const struct fs_operation fat32_op = {
	.fs_init_ptr = fat32_init,
	.file_init = filepnt_init,
	.file_create = createFile,
	.file_read = Fatfile_read,
	.file_write = Fatfile_write,
	.file_remove = rmfile,
	.makedir = makeDirAt
};

static void build_dirent_tree(Dirent *parent) {
	Dirent *child;
	int off = 0; // 当前读到的偏移位置
	int ret;

	filepnt_init(parent);
	while (1) {
		ret = dirGetDentFrom(parent, off, &child, &off, NULL);
		if (ret == 0) {
			// 读到末尾
			break;
		}
		//printk("get child: %s, parent: %s\n", child->name, parent->name);

		// 跳过.和..
		if (strncmp(child->name, ".          ", 11) == 0 ||
		    strncmp(child->name, "..         ", 11) == 0) {
			continue;
		}

		list_append(&parent->child_list,&child->dirent_tag);

		// 如果为目录，就向下一层递归
		if (child->type == DIRENT_DIR) {
			build_dirent_tree(child);
		}
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

	// 释放缓冲区
	bufRelease(buf);

	return 0;
}

/**
 * @brief 用fat32初始化一个文件系统，根目录记录在fs->root中
 */
void fat32_init(FileSystem* fs) 
{
	// 1. 以fs为单位初始化簇管理器
	printk("fat32 is initing...\n");
	strcpy(fs->name, "FAT32");
	ASSERT(partition_format(fs) == 0);
	

	// 2. 初始化根目录
	fs->root = dirent_alloc();
	
	strcpy(fs->root->name, "/");
	fs->root->file_system = fs; // 此句必须放在countCluster之前，用于设置fs
	printk("cluster Init Finished!\n");
	// 设置Dirent属性
	fs->root->first_clus = fs->superBlock.bpb.root_clus;
	printk("first clus of root is %d\n", fs->root->first_clus);
	fs->root->raw_dirent.DIR_Attr = ATTR_DIRECTORY;
	fs->root->raw_dirent.DIR_FileSize = 0; // 目录的Dirent的size都是0
	fs->root->type = DIRENT_DIR;
	fs->root->file_size = countClusters(fs->root) * CLUS_SIZE(fs);
	fs->op = &fat32_op;
	// 设置树状结构
	fs->root->parent_dirent = NULL; // 父节点为空，表示已经到达根节点
	list_init(&fs->root->child_list);

	fs->root->linkcnt = 1;

	/* 不需要初始化fs->root的锁，因为在分配时即初始化了 */

	printk("root directory init finished!\n");
	ASSERT(sizeof(FAT32Directory) == DIRENT_SIZE);

	// 3. 递归建立Dirent树
	build_dirent_tree(fs->root);
	printk("build dirent tree succeed!\n");
}


void init_root_fs(void) 
{
	//extern FileSystem *fatFs;
	lock_init(&mtx_file);
	//allocFs(&fatFs);
	allocFs(&ext4Fs);
	//fatFs->deviceNumber = 0;
	pr_info("ext4Fs->root alloc started\n");
	ext4Fs->root = dirent_alloc();
	pr_info("ext4Fs->root: 0x%p\n", ext4Fs->root);
	pr_info("ext4Fs->root alloc finished\n");
	mnt_root.mnt_rootdir = ext4Fs->root;
	ext4Fs->root->head = &mnt_root;
	pr_info("ext4_init start started\n");
	ext4_init(ext4Fs);
	pr_info("ext4_init finished\n");
	//fat32_init(fatFs);
	/*将原来的rootfs目录转移到fat32下*/
	
	/*将fat32系统挂载到根挂载点*/
	
}
