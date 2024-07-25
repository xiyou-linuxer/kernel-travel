#include <fs/ext4.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_sb.h>
#include <fs/ext4_block_group.h>
#include <fs/ext4_extent.h>
#include <fs/buf.h>
#include <xkernel/string.h>
#include <xkernel/stdio.h>
#include <fs/ext4_bitmap.h>

void ext4_inode_set_access_time(struct ext4_inode *inode, uint32_t time)
{
	inode->access_time = to_le32(time);
}

void ext4_inode_set_generation(struct ext4_inode *inode, uint32_t gen)
{
	inode->generation = to_le32(gen);
}

void ext4_inode_set_change_inode_time(struct ext4_inode *inode, uint32_t time)
{
	inode->change_inode_time = to_le32(time);
}

void ext4_inode_set_modif_time(struct ext4_inode *inode, uint32_t time)
{
	inode->modification_time = to_le32(time);
}

void ext4_inode_set_del_time(struct ext4_inode *inode, uint32_t time)
{
	inode->deletion_time = to_le32(time);
}

uint32_t ext4_inode_get_del_time(struct ext4_inode *inode)
{
	return to_le32(inode->deletion_time);
}

struct ext4_extent_header * ext4_inode_get_extent_header(struct ext4_inode *inode)
{
	return (struct ext4_extent_header *)inode->blocks;
}

void ext4_inode_set_csum(struct ext4_sblock *sb, struct ext4_inode *inode,uint32_t checksum)
{
	// 从超级块结构中获取 inode 的大小
	uint16_t inode_size = ext4_get16(sb, inode_size);

	// 将校验和的低 16 位存储到 inode 结构的 osd2.linux2.checksum_lo 字段中
	inode->osd2.linux2.checksum_lo =
		to_le16((checksum << 16) >> 16);

	// 如果 inode 大小大于 EXT4_GOOD_OLD_INODE_SIZE，则将校验和的高 16 位存储到 inode 结构的 checksum_hi 字段中
	if (inode_size > EXT4_GOOD_OLD_INODE_SIZE)
		inode->checksum_hi = to_le16(checksum >> 16);
}

// 从 inode 中获取校验和
uint32_t ext4_inode_get_csum(struct ext4_sblock *sb, struct ext4_inode *inode)
{
	uint16_t inode_size = ext4_get16(sb, inode_size); // 获取 inode 大小
	uint32_t v = to_le16(inode->osd2.linux2.checksum_lo); // 获取低位校验和
	if (inode_size > EXT4_GOOD_OLD_INODE_SIZE)
		v |= ((uint32_t)to_le16(inode->checksum_hi)) << 16; // 如果 inode 大小超过旧版大小，合并高位校验和
	return v;
}

static uint32_t ext4_ialloc_inode_to_bgidx(struct ext4_sblock *sb,
					   uint32_t inode)
{
	uint32_t inodes_per_group = ext4_get32(sb, inodes_per_group);
	return (inode - 1) % inodes_per_group;
}

void ext4_inode_set_extra_isize(struct ext4_sblock *sb,
				struct ext4_inode *inode,
				uint16_t size)
{
	uint16_t inode_size = ext4_get16(sb, inode_size);
	if (inode_size > EXT4_GOOD_OLD_INODE_SIZE)
		inode->extra_isize = to_le16(size);
}

void ext4_inode_set_links_cnt(struct ext4_inode *inode, uint16_t cnt)
{
	inode->links_count = to_le16(cnt);
}

uint32_t ext4_inode_get_direct_block(struct ext4_inode *inode, uint32_t idx)
{
	return to_le32(inode->blocks[idx]);
}

void ext4_inode_set_direct_block(struct ext4_inode *inode, uint32_t idx, uint32_t block)
{
	inode->blocks[idx] = to_le32(block);
}

uint32_t ext4_inode_get_indirect_block(struct ext4_inode *inode, uint32_t idx)
{
	return to_le32(inode->blocks[idx + EXT4_INODE_INDIRECT_BLOCK]);
}

void ext4_inode_set_indirect_block(struct ext4_inode *inode, uint32_t idx, uint32_t block)
{
	inode->blocks[idx + EXT4_INODE_INDIRECT_BLOCK] = to_le32(block);
}

// 获取 inode 的 generation 字段
uint32_t ext4_inode_get_generation(struct ext4_inode *inode)
{
	return to_le32(inode->generation);
}

// 获取 inode 的 flags 字段
uint32_t ext4_inode_get_flags(struct ext4_inode *inode)
{
    return to_le32(inode->flags); // 返回 inode 的 flags 字段，转为小端格式
}

bool ext4_inode_has_flag(struct ext4_inode *inode, uint32_t f)
{
	return ext4_inode_get_flags(inode) & f;
}

// 设置 inode 的 flags 字段
void ext4_inode_set_flags(struct ext4_inode *inode, uint32_t flags)
{
    inode->flags = to_le32(flags); // 将 flags 设置到 inode 中，转为小端格式
}

uint32_t ext4_inode_type(struct ext4_sblock *sb, struct ext4_inode *inode)
{
	return (ext4_inode_get_mode(sb, inode) & EXT4_INODE_MODE_TYPE_MASK);
}

// 检查 inode 的类型是否与给定类型相同
bool ext4_inode_is_type(struct ext4_sblock *sb, struct ext4_inode *inode, uint32_t type)
{
    return ext4_inode_type(sb, inode) == type; // 比较 inode 的类型与给定类型
}

// 清除 inode 的指定标志位
void ext4_inode_clear_flag(struct ext4_inode *inode, uint32_t f)
{
    uint32_t flags = ext4_inode_get_flags(inode); // 获取当前 flags
    flags = flags & (~f); // 清除指定的标志位
    ext4_inode_set_flags(inode, flags); // 将更新后的 flags 设置回 inode
}

static uint32_t ext4_ialloc_get_bgid_of_inode(struct ext4_sblock *sb,
					      uint32_t inode)
{
	uint32_t inodes_per_group = ext4_get32(sb, inodes_per_group);
	return (inode - 1) / inodes_per_group;
}

// 设置 inode 的指定标志位
void ext4_inode_set_flag(struct ext4_inode *inode, uint32_t f)
{
    uint32_t flags = ext4_inode_get_flags(inode); // 获取当前 flags
    flags = flags | f; // 设置指定的标志位
    ext4_inode_set_flags(inode, flags); // 将更新后的 flags 设置回 inode
}
/**
 * ext4_inode_block_bits_count - 计算块大小需要多少位
 * @block_size: 块大小（以字节为单位）
 *
 * 该函数用于计算给定块大小所需的位数。初始位数为8，然后每次将块大小右移1位，
 * 直到块大小不大于256。每次右移位数增加1。
 * 
 * 返回值是块大小所需的位数。
 */
static uint32_t ext4_inode_block_bits_count(uint32_t block_size)
{
	// 初始化位数为8
	uint32_t bits = 8;
	// 将块大小赋值给size变量
	uint32_t size = block_size;

	// 当块大小大于256时，循环执行
	do {
		// 增加位数
		bits++;
		// 块大小右移1位（相当于除以2）
		size = size >> 1;
	} while (size > 256);

	// 返回计算得到的位数
	return bits;
}

uint64_t ext4_inode_get_blocks_count(struct ext4_sblock *sb, struct ext4_inode *inode)
{
	// 从 inode 结构的 blocks_count_lo 字段读取块计数，并将其转换为小端序
	uint64_t cnt = to_le32(inode->blocks_count_lo);

	// 检查超级块中是否设置了只读兼容的 EXT4_FRO_COM_HUGE_FILE 特性
	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_HUGE_FILE)) {

		// 处理 48 位字段，将高 16 位（blocks_high）左移 32 位后与低 32 位（cnt）合并
		cnt |= (uint64_t)to_le16(inode->osd2.linux2.blocks_high) << 32;

		// 检查 inode 中是否设置了 EXT4_INODE_FLAG_HUGE_FILE 标志
		if (ext4_inode_has_flag(inode, EXT4_INODE_FLAG_HUGE_FILE)) {

			// 获取块大小和每块位数
			uint32_t block_count = ext4_sb_get_block_size(sb);
			uint32_t b = ext4_inode_block_bits_count(block_count);
			
			// 将块计数左移适当的位数以处理大文件
			return cnt << (b - 9);
		}
	}

	// 返回计算的块计数
	return cnt;
}

int ext4_inode_set_blocks_count(struct ext4_sblock *sb, struct ext4_inode *inode, uint64_t count)
{
	/* 32-bit maximum */
	uint64_t max = 0;
	max = ~max >> 32;

	if (count <= max) {
		inode->blocks_count_lo = to_le32((uint32_t)count);
		inode->osd2.linux2.blocks_high = 0;
		ext4_inode_clear_flag(inode, EXT4_INODE_FLAG_HUGE_FILE);

		return 0;
	}

	/* Check if there can be used huge files (many blocks) */
	if (!ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_HUGE_FILE))
		return -1;

	/* 48-bit maximum */
	max = 0;
	max = ~max >> 16;

	if (count <= max) {
		inode->blocks_count_lo = to_le32((uint32_t)count);
		inode->osd2.linux2.blocks_high = to_le16((uint16_t)(count >> 32));
		ext4_inode_clear_flag(inode, EXT4_INODE_FLAG_HUGE_FILE);
	} else {
		uint32_t block_count = ext4_sb_get_block_size(sb);
		uint32_t block_bits =ext4_inode_block_bits_count(block_count);

		ext4_inode_set_flag(inode, EXT4_INODE_FLAG_HUGE_FILE);
		count = count >> (block_bits - 9);
		inode->blocks_count_lo = to_le32((uint32_t)count);
		inode->osd2.linux2.blocks_high = to_le16((uint16_t)(count >> 32));
	}

	return 0;
}


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

uint32_t ext4_inode_get_gid(struct ext4_inode *inode)
{
	return to_le32(inode->gid);
}

void ext4_inode_set_gid(struct ext4_inode *inode, uint32_t gid)
{
	inode->gid = to_le32(gid);
}

uint64_t ext4_inode_get_size(struct ext4_sblock *sb, struct ext4_inode *inode)
{
	uint64_t v = to_le32(inode->size_lo);
	
	if ((ext4_get32(sb, rev_level) > 0) &&
	    (ext4_inode_is_type(sb, inode, EXT4_INODE_MODE_FILE)))
		v |= ((uint64_t)to_le32(inode->size_hi)) << 32;
	
	return v;
}

static uint32_t ext4_ialloc_bgidx_to_inode(struct ext4_sblock *sb,
					   uint32_t index, uint32_t bgid)
{
	uint32_t inodes_per_group = ext4_get32(sb, inodes_per_group);
	return bgid * inodes_per_group + (index + 1);
}

void ext4_inode_set_size(struct ext4_inode *inode, uint64_t size)
{
	inode->size_lo = to_le32((size << 32) >> 32);
	inode->size_hi = to_le32(size >> 32);
}

bool ext4_ialloc_verify_bitmap_csum(struct ext4_sblock *sb, struct ext4_bgroup *bg,
			       void *bitmap __attribute__ ((__unused__)))
{

	int desc_size = ext4_sb_get_desc_size(sb);
	uint32_t csum = ext4_ialloc_bitmap_csum(sb, bitmap);
	uint16_t lo_csum = to_le16(csum & 0xFFFF),
		 hi_csum = to_le16(csum >> 16);

	if (!ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM))
		return true;

	if (bg->inode_bitmap_csum_lo != lo_csum)
		return false;

	if (desc_size == EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE)
		if (bg->inode_bitmap_csum_hi != hi_csum)
			return false;

	return true;
}

#if CONFIG_META_CSUM_ENABLE
static bool ext4_fs_verify_inode_csum(struct ext4_inode_ref *inode_ref)
{
	struct ext4_sblock *sb = &inode_ref->fs->sb;
	if (!ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM))
		return true;

	return ext4_inode_get_csum(sb, inode_ref->inode) ==
	       ext4_fs_inode_checksum(inode_ref);
}
#else
#define ext4_fs_verify_inode_csum(...) true
#endif


/**
 * @brief 获取指定索引的inode引用
 * 
 * @param fs 文件系统指针
 * @param index inode索引
 * @param ref 存储inode引用的结构
 * @param initialized 是否需要初始化
 * @return 错误代码
 */
int ext4_fs_get_inode_ref(FileSystem *fs, uint32_t index,struct ext4_inode_ref *ref,bool initialized)
{
	// 计算一个数据块中可容纳的inode数量
	uint32_t inodes_per_group = ext4_get32(&fs->superBlock.ext4_sblock, inodes_per_group);
	// inode编号是从1开始的，但计算索引时使用0开始更简单
	index -= 1;
	uint32_t block_group = index / inodes_per_group;  // 所属块组
	uint32_t offset_in_group = index % inodes_per_group;  // 在块组中的偏移量

	// 加载inode所在的块组
	struct ext4_block_group_ref bg_ref;
	int rc = ext4_fs_get_block_group_ref(fs, block_group, &bg_ref);//获取块组的引用
	if (rc != 0) {
		return rc;
	}

	// 获取inode表所在的块地址
	ext4_fsblk_t inode_table_start = ext4_bg_get_inode_table_first_block(bg_ref.block_group, &fs->superBlock.ext4_sblock);

	// 释放块组引用（不再需要）
	rc = ext4_fs_put_block_group_ref(&bg_ref);
	if (rc != 0) {
		return rc;
	}
	
	// 计算inode在块组中的位置
	uint16_t inode_size = ext4_get16(&fs->superBlock.ext4_sblock, inode_size);
	uint32_t block_size = ext4_sb_get_block_size(&fs->superBlock.ext4_sblock);
	
	uint32_t byte_offset_in_group = offset_in_group * inode_size;

	// 计算块地址
	ext4_fsblk_t block_id =inode_table_start + (byte_offset_in_group / block_size);

	ref->block = bufRead(1,EXT4_LBA2PBA(block_id),1);
	// 计算inode在数据块中的位置
	uint32_t offset_in_block = byte_offset_in_group % block_size;
	ref->inode = (struct ext4_inode *)(ref->block->data->data + offset_in_block);

	if (offset_in_block>=512)
	{
		int n =offset_in_block/512;
		ref->block = bufRead(1,EXT4_LBA2PBA(block_id)+n,1);
		int off = offset_in_block % 512;
		ref->inode = (struct ext4_inode *)(ref->block->data->data + off);
	}
	
	// 需要在引用中存储原始的索引值
	ref->index = index + 1;
	ref->fs = fs;
	ref->dirty = false;

	// 如果需要初始化且校验和失败，记录警告
	if (initialized && !ext4_fs_verify_inode_csum(ref)) {
		printk("Inode checksum failed.""Inode: %d \n",
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
	return 1;
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
	switch (ext4_inode_type(&fs->superBlock.ext4_sblock, inode_ref->inode)) {
	case EXT4_INODE_MODE_FILE:
	case EXT4_INODE_MODE_DIRECTORY:
		break;
	default:
		return;
	}
	if (ext4_sb_feature_incom(&fs->superBlock.ext4_sblock, EXT4_FINCOM_EXTENTS)) {
		ext4_inode_set_flag(inode, EXT4_INODE_FLAG_EXTENTS);

		/* Initialize extent root header */
		ext4_extent_tree_init(inode_ref);
	}
	inode_ref->dirty = true;
}

int ext4_ialloc_alloc_inode(struct FileSystem *fs, uint32_t *idx, bool is_dir)
{
	int EOK = 0;  // 成功的返回值
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;

	// 获取文件系统的块组信息
	uint32_t bgid = fs->ext4_fs.last_inode_bg_id;
	uint32_t bg_count = ext4_block_group_cnt(sb);
	uint32_t sb_free_inodes = ext4_get32(sb, free_inodes_count);
	bool rewind = false;

	/* 尝试在所有块组中找到空闲的 inode */
	while (bgid <= bg_count) {

		if (bgid == bg_count) {
			if (rewind)
				break;
			// 如果已经遍历了所有块组，重置计数器重新遍历
			bg_count = fs->ext4_fs.last_inode_bg_id;
			bgid = 0;
			rewind = true;
			continue;
		}

		/* 加载块组以进行检查 */
		struct ext4_block_group_ref bg_ref;
		int rc = ext4_fs_get_block_group_ref(fs, bgid, &bg_ref);
		if (rc != EOK)
			return rc;

		struct ext4_bgroup *bg = bg_ref.block_group;

		/* 读取算法所需的值 */
		uint32_t free_inodes = ext4_bg_get_free_inodes_count(bg, sb);
		uint32_t used_dirs = ext4_bg_get_used_dirs_count(bg, sb);

		/* 检查该块组是否适合分配 */
		if (free_inodes > 0) {
			/* 加载带有位图的块 */
			ext4_fsblk_t bmp_blk_add = ext4_bg_get_inode_bitmap(bg, sb);

			struct ext4_block b;
			b.buf = bufRead(1,EXT4_LBA2PBA(bmp_blk_add),1);

			if (b.buf == NULL) {
				ext4_fs_put_block_group_ref(&bg_ref);
				return rc;
			}

			// 验证位图的校验和
			if (!ext4_ialloc_verify_bitmap_csum(sb, bg, b.buf->data->data)) {
				printk("Bitmap checksum failed.Group 1: %d \n",bg_ref.index);
			}

			/* 尝试在位图中分配 inode */
			uint32_t inodes_in_bg;
			uint32_t idx_in_bg;

			inodes_in_bg = ext4_inodes_in_group_cnt(sb, bgid);
			rc = ext4_bmap_bit_find_clr(b.buf->data->data, 0, inodes_in_bg,
						    &idx_in_bg);
			/* 如果块组中没有空闲的 inode */
			if (rc == 28) {
				bufRelease(b.buf);
				rc = ext4_fs_put_block_group_ref(&bg_ref);
				if (rc != EOK)
					return rc;

				continue;
			}

			// 设置找到的空闲 inode 位
			ext4_bmap_bit_set(b.buf->data->data, idx_in_bg);

			/* 找到空闲的 inode，保存位图 */
			ext4_ialloc_set_bitmap_csum(sb, bg, b.buf->data->data);
			bufWrite(b.buf);

			// 保存修改的块
			bufRelease(b.buf);
			
			/* 修改文件系统计数器 */
			free_inodes--;
			ext4_bg_set_free_inodes_count(bg, sb, free_inodes);

			/* 增加已使用目录计数器 */
			if (is_dir) {
				used_dirs++;
				ext4_bg_set_used_dirs_count(bg, sb, used_dirs);
			}

			/* 减少未使用的 inode 计数 */
			uint32_t unused = ext4_bg_get_itable_unused(bg, sb);
			uint32_t free = inodes_in_bg - unused;

			if (idx_in_bg >= free) {
				unused = inodes_in_bg - (idx_in_bg + 1);
				ext4_bg_set_itable_unused(bg, sb, unused);
			}

			/* 保存修改后的块组 */
			bg_ref.dirty = true;

			rc = ext4_fs_put_block_group_ref(&bg_ref);
			if (rc != EOK)
				return rc;

			/* 更新超级块 */
			sb_free_inodes--;
			ext4_set32(sb, free_inodes_count, sb_free_inodes);

			/* 计算绝对 inode 号 */
			*idx = ext4_ialloc_bgidx_to_inode(sb, idx_in_bg, bgid);

			 fs->ext4_fs.last_inode_bg_id = bgid;

			return EOK;
		}

		/* 如果块组未被修改，释放它并跳到下一个块组 */
		ext4_fs_put_block_group_ref(&bg_ref);
		if (rc != EOK)
			return rc;

		++bgid;
	}

	return -1;
}

int ext4_ialloc_free_inode(struct FileSystem *fs, uint32_t index, bool is_dir)
{
	struct ext4_sblock *sb = &fs->superBlock.ext4_sblock;
	int EOK= 0;
	/* Compute index of block group and load it */
	uint32_t block_group = ext4_ialloc_get_bgid_of_inode(sb, index);

	struct ext4_block_group_ref bg_ref;
	int rc = ext4_fs_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK)
		return rc;

	struct ext4_bgroup *bg = bg_ref.block_group;

	/* Load i-node bitmap */
	ext4_fsblk_t bitmap_block_addr =
	    ext4_bg_get_inode_bitmap(bg, sb);

	struct ext4_block b;
	b.buf = bufRead(1,EXT4_LBA2PBA(bitmap_block_addr),1);

	if (!ext4_ialloc_verify_bitmap_csum(sb, bg, b.buf->data->data)) {
		printk("Bitmap checksum failed.Group 2: %d \n",bg_ref.index);
	}

	/* Free i-node in the bitmap */
	uint32_t index_in_group = ext4_ialloc_inode_to_bgidx(sb, index);
	ext4_bmap_bit_clr(b.buf->data->data, index_in_group);
	ext4_ialloc_set_bitmap_csum(sb, bg, b.buf->data->data);
	ext4_trans_set_block_dirty(b.buf);

	/* Put back the block with bitmap */
	bufRelease(b.buf);

	/* If released i-node is a directory, decrement used directories count
	 */
	if (is_dir) {
		uint32_t bg_used_dirs = ext4_bg_get_used_dirs_count(bg, sb);
		bg_used_dirs--;
		ext4_bg_set_used_dirs_count(bg, sb, bg_used_dirs);
	}

	/* Update block group free inodes count */
	uint32_t free_inodes = ext4_bg_get_free_inodes_count(bg, sb);
	free_inodes++;
	ext4_bg_set_free_inodes_count(bg, sb, free_inodes);

	bg_ref.dirty = true;

	/* Put back the modified block group */
	rc = ext4_fs_put_block_group_ref(&bg_ref);
	if (rc != EOK)
		return rc;

	/* Update superblock free inodes count */
	ext4_set32(sb, free_inodes_count,
		   ext4_get32(sb, free_inodes_count) + 1);

	return EOK;
}

/*分配一个inode节点*/
int ext4_fs_alloc_inode(struct FileSystem *fs, struct ext4_inode_ref *inode_ref, int filetype)
{
	/* 检查新分配的 inode 是否是目录 */
	int EOK = 0;
	bool is_dir;
	uint16_t inode_size = ext4_get16(&ext4Fs->superBlock.ext4_sblock, inode_size);

	is_dir = (filetype == EXT4_DE_DIR);

	/* 通过分配算法分配 inode */
	uint32_t index;
	int rc = ext4_ialloc_alloc_inode(fs, &index, is_dir);
	if (rc != EOK)
		return rc;

	/* 从磁盘上的 inode 表加载 inode */
	rc = ext4_fs_get_inode_ref(fs, index, inode_ref, false);
	if (rc != EOK) {
		ext4_ialloc_free_inode(fs, index, is_dir);
		return rc;
	}

	/* 初始化 inode */
	struct ext4_inode *inode = inode_ref->inode;

	memset(inode, 0, inode_size);

	uint32_t mode;
	if (is_dir) {
		/*
		 * 为了与其他系统兼容，目录的默认权限为 0777 (八进制) == rwxrwxrwx
		 */
		mode = 0777;
		mode |= EXT4_INODE_MODE_DIRECTORY;
	} else if (filetype == EXT4_DE_SYMLINK) {
		/*
		 * 为了与其他系统兼容，符号链接的默认权限为 0777 (八进制) == rwxrwxrwx
		 */
		mode = 0777;
		mode |= EXT4_INODE_MODE_SOFTLINK;
	} else {
		/*
		 * 为了与其他系统兼容，文件的默认权限为 0666 (八进制) == rw-rw-rw-
		 */
		mode = 0666;
		mode |= ext4_fs_correspond_inode_mode(filetype);
	}
	ext4_inode_set_mode(&ext4Fs->superBlock.ext4_sblock, inode, mode);

	/* 初始化其他 inode 字段 */
	ext4_inode_set_links_cnt(inode, 0);
	ext4_inode_set_uid(inode, 0);
	ext4_inode_set_gid(inode, 0);
	ext4_inode_set_size(inode, 0);
	ext4_inode_set_access_time(inode, 0);
	ext4_inode_set_change_inode_time(inode, 0);
	ext4_inode_set_modif_time(inode, 0);
	ext4_inode_set_del_time(inode, 0);
	ext4_inode_set_blocks_count(&ext4Fs->superBlock.ext4_sblock, inode, 0);
	ext4_inode_set_flags(inode, 0);
	ext4_inode_set_generation(inode, 0);
	if (inode_size > EXT4_GOOD_OLD_INODE_SIZE) {
		uint16_t size = ext4_get16(&ext4Fs->superBlock.ext4_sblock, want_extra_isize);
		ext4_inode_set_extra_isize(&ext4Fs->superBlock.ext4_sblock, inode, size);
	}

	memset(inode->blocks, 0, sizeof(inode->blocks));
	inode_ref->dirty = true;

	return EOK;
}
