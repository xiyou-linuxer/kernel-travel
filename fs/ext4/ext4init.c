#include <fs/fs.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/mount.h>
#include <fs/buf.h>
#include <fs/ext4.h>
#include <sync.h>
#include <debug.h>
FileSystem *ext4Fs;
static const struct fs_operation ext4_op = {
	ext4_init
};

static void build_dirent_ext4tree(Dirent *parent)
{
	
}

/**
 * @brief 初始化分区信息，填写文件系统结构体里面的超级块
 */
int fill_sb(FileSystem *fs)
{
	ASSERT(fs != NULL);
	ASSERT(fs->get != NULL);
	Buffer *buf = fs->get(fs, 1, true);//磁盘中ext4超级块位于第一扇区
	if (buf == NULL) {
		printk("buf == NULL\n");
		return -E_DEV_ERROR;
	}
	/*填写超级块中ext4的基本属性*/
	struct ext4_sblock *ext4_sblock = (struct ext4_sblock *)(buf->data->data);
	fs->superBlock.ext4_sblock.inodes_count = ext4_sblock->inodes_count;
	fs->superBlock.ext4_sblock.blocks_count_lo = ext4_sblock->blocks_count_lo;
	fs->superBlock.ext4_sblock.reserved_blocks_count_lo = ext4_sblock->reserved_blocks_count_lo;
	fs->superBlock.ext4_sblock.free_blocks_count_lo = ext4_sblock->free_blocks_count_lo;
	fs->superBlock.ext4_sblock.free_inodes_count = ext4_sblock->free_inodes_count;
	fs->superBlock.ext4_sblock.first_data_block = ext4_sblock->first_data_block;
	fs->superBlock.ext4_sblock.log_block_size = ext4_sblock->log_block_size;
	fs->superBlock.ext4_sblock.log_cluster_size = ext4_sblock->log_cluster_size;
	fs->superBlock.ext4_sblock.blocks_per_group = ext4_sblock->blocks_per_group;
	fs->superBlock.ext4_sblock.frags_per_group = ext4_sblock->frags_per_group;
	fs->superBlock.ext4_sblock.inodes_per_group = ext4_sblock->inodes_per_group;
	fs->superBlock.ext4_sblock.mount_time = ext4_sblock->mount_time;
	fs->superBlock.ext4_sblock.write_time = ext4_sblock->write_time;
	fs->superBlock.ext4_sblock.mount_count = ext4_sblock->mount_count;
	fs->superBlock.ext4_sblock.max_mount_count = ext4_sblock->max_mount_count;
	fs->superBlock.ext4_sblock.magic = ext4_sblock->magic;
	fs->superBlock.ext4_sblock.state = ext4_sblock->state;
	fs->superBlock.ext4_sblock.errors = ext4_sblock->errors;
	fs->superBlock.ext4_sblock.minor_rev_level = ext4_sblock->minor_rev_level;
	fs->superBlock.ext4_sblock.last_check_time = ext4_sblock->last_check_time;
	fs->superBlock.ext4_sblock.check_interval = ext4_sblock->check_interval;
	fs->superBlock.ext4_sblock.creator_os = ext4_sblock->creator_os;
	fs->superBlock.ext4_sblock.rev_level = ext4_sblock->rev_level;
	fs->superBlock.ext4_sblock.def_resuid = ext4_sblock->def_resuid;
	fs->superBlock.ext4_sblock.def_resgid = ext4_sblock->def_resgid;
	bufRelease(buf);
}

void ext4_init(void)
{
	allocFs(&ext4Fs);
	printk("ext4 is initing...\n");
	strcpy(ext4Fs->name, "ext4");
	ext4Fs->get = bufRead;
	//初始化超级块信息
	ASSERT(fill_sb(ext4Fs) == 0);
	
	//初始化根目录
	ext4Fs->root = dirent_alloc();
	strcpy(ext4Fs->root->name,"/");
	ext4Fs->root->file_system = ext4Fs;
	ext4Fs->root->first_clus = ext4Fs->superBlock.ext4_sblock.first_data_block;/* 根目录项在第一个数据块上 */
	ext4Fs->op = &ext4_op;
	ext4Fs->root->type = DIRENT_DIR;
	//设置目录树的树根
	ext4Fs->root->parent_dirent = NULL; // 父节点为空，表示已经到达根节点
	list_init(&ext4Fs->root->child_list);
	ext4Fs->root->linkcnt = 1;
	//构建目录树
	build_dirent_ext4tree(ext4Fs->root);
	printk("build dirent tree succeed!\n");
}