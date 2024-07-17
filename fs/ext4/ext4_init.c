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
#include <fs/ext4_fs.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_file.h>
#include <sync.h>
#include <debug.h>
FileSystem *ext4Fs;
static const struct fs_operation ext4_op = {
 	.fs_init_ptr = ext4_init,
	.file_read = ext4_fread,
	.file_write = ext4_fwrite
};

static void build_dirent_ext4tree(Dirent *parent)
{
	Dirent *child;
	int ret;
	struct ext4_dir d;
	d.pdirent = parent;
	d.next_off = 0;
	printk("parent %s:\n",parent->name);
	while (1) {
		//获取目录项
		child = ext4_dir_entry_next(&d);
		printk("%s_inode:%d ",child->name,child->ext4_dir_en.inode);
		if (child == NULL) {
			// 读到末尾
			break;
		}
		// 跳过.和..
		if (strncmp(child->name, ".", 2) == 0 ||
		    strncmp(child->name, "..", 3) == 0) {
			continue;
		}
		list_append(&parent->child_list,&child->dirent_tag);
		
		// 如果为目录，就向下一层递归
		if (child->type == DIRENT_DIR) {
			printk("\n");
			build_dirent_ext4tree(child);
		}
	}
}

/**
 * @brief 初始化分区信息，填写文件系统结构体里面的超级块
 */
int fill_sb(FileSystem *fs)
{
	ASSERT(fs != NULL);

	/*填写超级块中ext4的基本属性*/
	Buffer *buf0 = bufRead(1,2,1);//磁盘中ext4超级块位于第二扇区
	void *p = &fs->superBlock.ext4_sblock;
	memcpy(p,buf0->data->data,512);
	bufRelease(buf0);
	Buffer *buf1 = bufRead(1,3,1);
	memcpy(p+512,buf1->data->data,512);
	bufRelease(buf1);
	uint16_t tmp;
	// 检查超级块是否有效
	if (!ext4_sb_check(&fs->superBlock.ext4_sblock))
		return -1;
	// 从超级块获取块大小
	uint32_t bsize = ext4_sb_get_block_size(&fs->superBlock.ext4_sblock);
	if (bsize > EXT4_MAX_BLOCK_SIZE)
		return -1;
	// 检查文件系统特性并在必要时更新只读标志
	int r = ext4_fs_check_features(&fs->ext4_fs);
	if (r != 0)
		return r;
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
	tmp = ext4_get16(&fs->superBlock.ext4_sblock, state);
	if (tmp & EXT4_SUPERBLOCK_STATE_ERROR_FS)
		printk("上次卸载错误：超级块 fs_error 标志\n");
	// 标记文件系统为已挂载
	ext4_set16(&fs->superBlock.ext4_sblock, state, EXT4_SUPERBLOCK_STATE_ERROR_FS);
	// 更新超级块中的挂载计数
	ext4_set16(&fs->superBlock.ext4_sblock, mount_count,ext4_get16(&fs->superBlock.ext4_sblock, mount_count) + 1);
	return 0;
}

void ext4_init(FileSystem* fs)
{
	printk("ext4 is initing...\n");
	strcpy(ext4Fs->name, "ext4");
	//初始化超级块信息
	ASSERT(fill_sb(ext4Fs) != 0);
	//初始化根目录
	ext4Fs->root = dirent_alloc();
	ext4Fs->root->ext4_dir_en.inode = EXT4_ROOT_INO;//根目录对应的inode号
	strcpy(ext4Fs->root->name,"/");
	ext4Fs->root->file_system = ext4Fs;
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