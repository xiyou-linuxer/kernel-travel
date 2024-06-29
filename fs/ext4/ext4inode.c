#include <fs/ext4.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/buf.h>
#include <xkernel/stdio.h>
/**
 * @brief 获取 inode 的模式（文件类型和权限）
 *
 * @param sb 文件系统的超级块
 * @param inode 目标 inode
 * @return uint32_t inode 的模式（文件类型和权限）
 */
uint32_t ext4_inode_get_mode(struct ext4_sblock *sb, struct ext4_inode *inode)
{
	uint32_t v = to_le16(inode->mode); // 获取并转换 inode 的 mode 字段

	// 如果文件系统是由 HURD 操作系统创建的，则获取高 16 位的 mode
	if (ext4_get32(sb, creator_os) == EXT4_SUPERBLOCK_OS_HURD) {
		v |= ((uint32_t)to_le16(inode->osd2.hurd2.mode_high)) << 16;
	}

	return v; // 返回完整的 mode 值
}

/**
 * @brief 设置 inode 的模式（文件类型和权限）
 *
 * @param sb 文件系统的超级块
 * @param inode 目标 inode
 * @param mode 要设置的模式（文件类型和权限）
 */
void ext4_inode_set_mode(struct ext4_sblock *sb, struct ext4_inode *inode,
			 uint32_t mode)
{
	inode->mode = to_le16((mode << 16) >> 16); // 设置并转换 mode 的低 16 位

	// 如果文件系统是由 HURD 操作系统创建的，则设置 mode 的高 16 位
	if (ext4_get32(sb, creator_os) == EXT4_SUPERBLOCK_OS_HURD)
		inode->osd2.hurd2.mode_high = to_le16(mode >> 16);
}

uint32_t ext4_inode_get_uid(struct ext4_inode *inode)
{
	return to_le32(inode->uid);
}

void ext4_inode_set_uid(struct ext4_inode *inode, uint32_t uid)
{
	inode->uid = to_le32(uid);
}

uint64_t ext4_inode_get_size(struct ext4_sblock *sb, struct ext4_inode *inode)
{
	uint64_t v = to_le32(inode->size_lo);

	if ((ext4_get32(sb, rev_level) > 0) &&
	    (ext4_inode_is_type(sb, inode, EXT4_INODE_MODE_FILE)))
		v |= ((uint64_t)to_le32(inode->size_hi)) << 32;

	return v;
}

/**
 * @brief 获取指定索引的inode引用
 * 
 * @param fs 文件系统指针
 * @param index inode索引
 * @param ref 存储inode引用的结构
 * @param initialized 是否需要初始化
 * @return 错误代码
 */
int ext4_fs_get_inode_ref(FileSystem *fs, uint32_t index,
			struct ext4_inode_ref *ref,
			bool initialized)
{
	// 计算一个数据块中可容纳的inode数量
	uint32_t inodes_per_group = ext4_get32(&fs->superBlock.ext4_sblock, inodes_per_group);

	// inode编号是从1开始的，但计算索引时使用0开始更简单
	index -= 1;
	uint32_t block_group = index / inodes_per_group;  // 所属块组
	uint32_t offset_in_group = index % inodes_per_group;  // 在块组中的偏移量

	// 加载inode所在的块组
	struct ext4_block_group_ref bg_ref;
	int rc = ext4_fs_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != 0) {
		return rc;
	}

	// 获取inode表所在的块地址
	ext4_fsblk_t inode_table_start =
	    ext4_bg_get_inode_table_first_block(bg_ref.block_group, &fs->sb);

	// 释放块组引用（不再需要）
	rc = ext4_fs_put_block_group_ref(&bg_ref);
	if (rc != 0) {
		return rc;
	}

	// 计算inode在块组中的位置
	uint16_t inode_size = ext4_get16(&fs->sb, inode_size);
	uint32_t block_size = ext4_sb_get_block_size(&fs->sb);
	uint32_t byte_offset_in_group = offset_in_group * inode_size;

	// 计算块地址
	ext4_fsblk_t block_id =
	    inode_table_start + (byte_offset_in_group / block_size);

	rc = ext4_trans_block_get(fs->bdev, &ref->block, block_id);
	if (rc != 0) {
		return rc;
	}

	// 计算inode在数据块中的位置
	uint32_t offset_in_block = byte_offset_in_group % block_size;
	ref->inode = (struct ext4_inode *)(ref->block.data + offset_in_block);

	// 需要在引用中存储原始的索引值
	ref->index = index + 1;
	ref->fs = fs;
	ref->dirty = false;

	// 如果需要初始化且校验和失败，记录警告
	if (initialized && !ext4_fs_verify_inode_csum(ref)) {
		printk("Inode checksum failed.""Inode: %" PRIu32"\n",
			ref->index);
	}

	return 0;
}

/**
 * ext4_fs_put_inode_ref - 将 inode 引用写回磁盘
 * @ref: 指向 ext4_inode_ref 结构的指针
 *
 * 如果引用被修改，则标记块为脏以便将更改写入物理设备。
 * 返回将包含 inode 的块放回到磁盘的结果。
 */
int ext4_fs_put_inode_ref(struct ext4_inode_ref *ref)
{
	/* 检查引用是否被修改 */
	if (ref->dirty) {
		/* 计算并设置 inode 的校验和 */
		ext4_fs_set_inode_checksum(ref);
		/* 标记块为脏，以便将更改写入物理设备 */
		bufWrite(ref->block);
	}
	
	/* 放回包含 inode 的块到磁盘 */
	return ext4_block_set(ref->fs->bdev, &ref->block);
}

/**
 * ext4_fs_inode_blocks_init - 初始化 inode 的块
 * @fs: 指向 ext4_fs 结构的指针，表示文件系统实例
 * @inode_ref: 指向 ext4_inode_ref 结构的指针，表示要初始化的 inode 引用
 *
 * 根据 inode 的类型进行初始化。如果不是文件或目录，则只填充块数组为 0。
 * 如果启用了扩展属性（extents），则初始化扩展属性并设置相应标志。
 */
void ext4_fs_inode_blocks_init(struct FileSystem *fs,
			       struct ext4_inode_ref *inode_ref)
{
	struct ext4_inode *inode = inode_ref->inode;

	/* 重置块数组。对于不是文件或目录的 inode，直接填充块数组为 0 */
	switch (ext4_inode_type(&fs->superBlock, inode_ref->inode)) {
	case EXT4_INODE_MODE_FILE:
	case EXT4_INODE_MODE_DIRECTORY:
		break;
	default:
		return;
	}
}
