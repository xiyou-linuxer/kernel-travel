#include <fs/fs.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/mount.h>
#include <fs/buf.h>
#include <fs/ext4.h>
#include <fs/ext4_sb.h>
#include <sync.h>
#include <debug.h>
FileSystem *ext4Fs;
static const struct fs_operation ext4_op = {
	ext4_init
};

static void build_dirent_ext4tree(Dirent *parent)
{
	Dirent *child;
	int off = 0; // 当前读到的偏移位置
	int ret;
	
	while (1) {
		//获取目录项
		ret = ext4_dir_entry_next();
		if (ret == 0) {
			// 读到末尾
			break;
		}

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

	uint16_t tmp;
	// 检查超级块是否有效
	if (!ext4_sb_check(&ext4Fs->superBlock.ext4_sblock))
		return -1;
	// 从超级块获取块大小
	uint32_t bsize = ext4_sb_get_block_size(&ext4Fs->superBlock.ext4_sblock);
	if (bsize > EXT4_MAX_BLOCK_SIZE)
		return -1;
	// 计算间接块级别的限制
	uint32_t blocks_id = bsize / sizeof(uint32_t);

	// 设置初始块限制和 inode 的每级块数
	ext4Fs->ext4_fs.inode_block_limits[0] = EXT4_INODE_DIRECT_BLOCK_COUNT;
	ext4Fs->ext4_fs.inode_blocks_per_level[0] = 1;

	// 计算接下来三个级别的块限制和每级块数
	for (int i = 1; i < 4; i++) {
		ext4Fs->ext4_fs.inode_blocks_per_level[i] =
		    ext4Fs->ext4_fs.inode_blocks_per_level[i - 1] * blocks_id;
		ext4Fs->ext4_fs.inode_block_limits[i] = ext4Fs->ext4_fs.inode_block_limits[i - 1] +
					    ext4Fs->ext4_fs.inode_blocks_per_level[i];
	}
	// 验证文件系统状态
	tmp = ext4_get16(&ext4Fs->superBlock.ext4_sblock, state);
	if (tmp & EXT4_SUPERBLOCK_STATE_ERROR_FS)
		printk("上次卸载错误：超级块 fs_error 标志\n");
	// 标记文件系统为已挂载
	ext4_set16(ext4_sblock, state, EXT4_SUPERBLOCK_STATE_ERROR_FS);
	// 更新超级块中的挂载计数
	ext4_set16(ext4_sblock, mount_count,ext4_get16(&fs->superBlock.ext4_sblock, mount_count) + 1);
	bufRelease(buf);
	return 0;
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