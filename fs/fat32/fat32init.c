#include <fs/fs.h>
#include <linux/types.h>
#include <linux/stdio.h>
#include <debug.h>
#include <linux/string.h>
#include <fs/fat32.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/filepnt.h>
#include <fs/cluster.h>
#include <sync.h>

FileSystem *fatFs;
struct lock mtx_file;
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
		list_append(&parent->child_list,&child->dirent_tag);
		// 跳过.和..防止死循环
		if (strncmp(child->name, ".          ", 11) == 0 ||
		    strncmp(child->name, "..         ", 11) == 0) {
			continue;
		}
		// 如果为目录，就向下一层递归
		if (child->type == DIRENT_DIR) {
			build_dirent_tree(child);
		}
	}
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
	fs->root->parent_dirent = fs->root;
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
	printk("fat32 init finished!\n");
}


void init_root_fs(void) 
{
	//extern FileSystem *fatFs;
	lock_init(&mtx_file);
	allocFs(&fatFs);

	fatFs->image = NULL;
	fatFs->deviceNumber = 0;

	fat32_init(fatFs);
}