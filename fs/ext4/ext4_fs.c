#include <fs/ext4.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_block_group.h>
#include <fs/ext4_sb.h>
#include <fs/ext4_crc32.h>
#include <fs/ext4_bitmap.h>
#include <fs/ext4_config.h>
#include <xkernel/debug.h>
#include <fs/buf.h>
#include <fs/fs.h>
#include <fs/ext4_extent.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <debug.h>

static void ext4_fs_mark_bitmap_end(int start_bit, int end_bit, void *bitmap)
{
	int i;

	if (start_bit >= end_bit)
		return;

	for (i = start_bit; (unsigned)i < ((start_bit + 7) & ~7UL); i++)
		ext4_bmap_bit_set(bitmap, i);

	if (i < end_bit)
		memset((char *)bitmap + (i >> 3), 0xff, (end_bit - i) >> 3);
}

static bool ext4_block_in_group(struct ext4_sblock *s, ext4_fsblk_t baddr,
				uint32_t bgid)
{
	uint32_t actual_bgid;
	actual_bgid = ext4_balloc_get_bgid_of_block(s, baddr);
	if (actual_bgid == bgid)
		return true;
	return false;
}

static uint16_t ext4_fs_bg_checksum(struct ext4_sblock *sb, uint32_t bgid,
				    struct ext4_bgroup *bg)
{
	/* If checksum not supported, 0 will be returned */
	uint16_t crc = 0;
#if CONFIG_META_CSUM_ENABLE
	/* Compute the checksum only if the filesystem supports it */
	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM)) {
		/* Use metadata_csum algorithm instead */
		uint32_t le32_bgid = to_le32(bgid);
		uint32_t orig_checksum, checksum;

		/* Preparation: temporarily set bg checksum to 0 */
		orig_checksum = bg->checksum;
		bg->checksum = 0;

		/* First calculate crc32 checksum against fs uuid */
		checksum =
		    ext4_crc32c(EXT4_CRC32_INIT, sb->uuid, sizeof(sb->uuid));
		/* Then calculate crc32 checksum against bgid */
		checksum = ext4_crc32c(checksum, &le32_bgid, sizeof(bgid));
		/* Finally calculate crc32 checksum against block_group_desc */
		checksum = ext4_crc32c(checksum, bg, ext4_sb_get_desc_size(sb));
		bg->checksum = orig_checksum;

		crc = checksum & 0xFFFF;
		return crc;
	}
#endif
	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_GDT_CSUM)) {
		uint8_t *base = (uint8_t *)bg;
		uint8_t *checksum = (uint8_t *)&bg->checksum;

		uint32_t offset = (uint32_t)(checksum - base);

		/* Convert block group index to little endian */
		uint32_t group = to_le32(bgid);

		/* Initialization */
		crc = ext4_bg_crc16(~0, sb->uuid, sizeof(sb->uuid));

		/* Include index of block group */
		crc = ext4_bg_crc16(crc, (uint8_t *)&group, sizeof(group));

		/* Compute crc from the first part (stop before checksum field)
		 */
		crc = ext4_bg_crc16(crc, (uint8_t *)bg, offset);

		/* Skip checksum */
		offset += sizeof(bg->checksum);

		/* Checksum of the rest of block group descriptor */
		if ((ext4_sb_feature_incom(sb, EXT4_FINCOM_64BIT)) &&
		    (offset < ext4_sb_get_desc_size(sb))) {

			const uint8_t *start = ((uint8_t *)bg) + offset;
			size_t len = ext4_sb_get_desc_size(sb) - offset;
			crc = ext4_bg_crc16(crc, start, len);
		}
	}
	return crc;
}

int ext4_fs_put_block_group_ref(struct ext4_block_group_ref *ref)
{
	// 检查引用是否被修改
	if (ref->dirty) {
		uint16_t cs;

		// 计算块组的新校验和
		cs = ext4_fs_bg_checksum(&ext4Fs->superBlock.ext4_sblock, ref->index, ref->block_group);
		ref->block_group->checksum = to_le16(cs);

		// 标记块为脏块以便将更改写入物理设备
		bufWrite(ref->block.buf);
	}
	/*释放缓冲区*/
	bufRelease(ref->block.buf);
	return 0;
}
/**
 * ext4_fs_get_descriptor_block - 获取块组描述符所在的块号
 * @s: 超级块指针
 * @bgid: 块组ID
 * @dsc_per_block: 每个块中的描述符数量
 *
 * 该函数根据块组ID和每个块中的描述符数量计算并返回块组描述符所在的块号。
 *
 * 返回值：块组描述符所在的块号
 */
static ext4_fsblk_t ext4_fs_get_descriptor_block(struct ext4_sblock *s, uint32_t bgid, uint32_t dsc_per_block)
{
	uint32_t first_meta_bg, dsc_id;
	int has_super = 0;

	// 计算描述符ID
	dsc_id = bgid / dsc_per_block;

	// 获取第一个元数据块组
	first_meta_bg = ext4_sb_first_meta_bg(s);

	// 检查是否启用了元数据块组特性
	bool meta_bg = ext4_sb_feature_incom(s, EXT4_FINCOM_META_BG);

	// 如果未启用元数据块组特性，或者描述符ID小于第一个元数据块组
	if (!meta_bg || dsc_id < first_meta_bg)
		// 返回第一个数据块 + 描述符ID + 1
		return ext4_get32(s, first_data_block) + dsc_id + 1;

	// 检查块组中是否包含超级块
	if (ext4_sb_is_super_in_bg(s, bgid))
		has_super = 1;
	// 返回超级块偏移量 + 块组的第一个块号
	return (has_super + ext4_fs_first_bg_block_no(s, bgid));
}

/**
 * @brief 初始化块组中的块位图
 * @param bg_ref 块组的引用
 * @return 错误码
 */
static int ext4_fs_init_block_bitmap(struct ext4_block_group_ref *bg_ref)
{
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
	struct ext4_bgroup *bg = bg_ref->block_group;
	int rc;

	uint32_t bit, bit_max;
	uint32_t group_blocks;
	uint16_t inode_size = ext4_get16(sb, inode_size);
	uint32_t block_size = ext4_sb_get_block_size(sb);
	uint32_t inodes_per_group = ext4_get32(sb, inodes_per_group);

	ext4_fsblk_t i;
	ext4_fsblk_t bmp_blk = ext4_bg_get_block_bitmap(bg, sb);
	ext4_fsblk_t bmp_inode = ext4_bg_get_inode_bitmap(bg, sb);
	ext4_fsblk_t inode_table = ext4_bg_get_inode_table_first_block(bg, sb);
	ext4_fsblk_t first_bg = ext4_balloc_get_block_of_bgid(sb, bg_ref->index);

	uint32_t dsc_per_block = block_size / ext4_sb_get_desc_size(sb);

	bool flex_bg = ext4_sb_feature_incom(sb, EXT4_FINCOM_FLEX_BG);
	bool meta_bg = ext4_sb_feature_incom(sb, EXT4_FINCOM_META_BG);

	uint32_t inode_table_bcnt = inodes_per_group * inode_size / block_size;

	struct ext4_block block_bitmap;
	//获取数据块，因为全部清0,所以不用从磁盘清0,直接在内存中获取buf
	bufRead(1,EXT4_LBA2PBA(bmp_blk),0);
	// 初始化块位图的内容为0
	memset(block_bitmap.buf->data->data, 0, block_size);
	bit_max = ext4_sb_is_super_in_bg(sb, bg_ref->index);

	uint32_t count = ext4_sb_first_meta_bg(sb) * dsc_per_block;
	if (!meta_bg || bg_ref->index < count) {
		if (bit_max) {
			bit_max += ext4_bg_num_gdb(sb, bg_ref->index);
			bit_max += ext4_get16(sb, s_reserved_gdt_blocks);
		}
	} else { // 对于 META_BG_BLOCK_GROUPS
		bit_max += ext4_bg_num_gdb(sb, bg_ref->index);
	}

	// 设置块位图中前 bit_max 位为 1
	for (bit = 0; bit < bit_max; bit++)
		ext4_bmap_bit_set(block_bitmap.buf->data->data, bit);

	if (bg_ref->index == ext4_block_group_cnt(sb) - 1) {
		/*
		 * 即使 mke2fs 总是初始化第一个和最后一个组，
		 * 如果其他工具启用了 EXT4_BG_BLOCK_UNINIT，我们需要确保计算正确的空闲块数
		 */
		group_blocks = (uint32_t)(ext4_sb_get_blocks_cnt(sb) -
					  ext4_get32(sb, first_data_block) -
					  ext4_get32(sb, blocks_per_group) *
					      (ext4_block_group_cnt(sb) - 1));
	} else {
		group_blocks = ext4_get32(sb, blocks_per_group);
	}

	bool in_bg;
	in_bg = ext4_block_in_group(sb, bmp_blk, bg_ref->index);
	if (!flex_bg || in_bg)
		ext4_bmap_bit_set(block_bitmap.buf->data->data, (uint32_t)(bmp_blk - first_bg));

	in_bg = ext4_block_in_group(sb, bmp_inode, bg_ref->index);
	if (!flex_bg || in_bg)
		ext4_bmap_bit_set(block_bitmap.buf->data->data, (uint32_t)(bmp_inode - first_bg));

	// 设置 inode 表中所有块在块位图中的对应位为 1
	for (i = inode_table; i < inode_table + inode_table_bcnt; i++) {
		in_bg = ext4_block_in_group(sb, i, bg_ref->index);
		if (!flex_bg || in_bg)
			ext4_bmap_bit_set(block_bitmap.buf->data->data, (uint32_t)(i - first_bg));
	}

	/*
	 * 如果组内的块数小于块大小 * 8（这是位图的大小），则将块位图的其余部分设置为1
	 */
	ext4_fs_mark_bitmap_end(group_blocks, block_size * 8, block_bitmap.buf->data->data);
	ext4_trans_set_block_dirty(block_bitmap.buf);

	// 设置块组的位图校验和
	ext4_balloc_set_bitmap_csum(sb, bg_ref->block_group, block_bitmap.buf->data->data);
	bg_ref->dirty = true;

	// 保存位图
	bufWrite(block_bitmap.buf);
	bufRelease(block_bitmap.buf);
	return 0;
}

void ext4_ialloc_set_bitmap_csum(struct ext4_sblock *sb, struct ext4_bgroup *bg, void *bitmap __attribute__ ((__unused__)))
{
	int desc_size = ext4_sb_get_desc_size(sb);
	uint32_t csum = ext4_ialloc_bitmap_csum(sb, bitmap);
	uint16_t lo_csum = to_le16(csum & 0xFFFF),
		 hi_csum = to_le16(csum >> 16);

	if (!ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM))
		return;

	/* See if we need to assign a 32bit checksum */
	bg->inode_bitmap_csum_lo = lo_csum;
	if (desc_size == EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->inode_bitmap_csum_hi = hi_csum;

}

/** @brief 初始化块组中的 i-node 位图。
 *  @param bg_ref 块组引用
 *  @return 错误代码
 */
static int ext4_fs_init_inode_bitmap(struct ext4_block_group_ref *bg_ref)
{
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
	struct ext4_bgroup *bg = bg_ref->block_group;

	/* 获取 inode 位图所在的磁盘扇区号 */
	ext4_fsblk_t bitmap_block_addr = ext4_bg_get_inode_bitmap(bg, sb);

	struct ext4_block b;
	/*获取数据块*/
	b.buf = bufRead(1,EXT4_LBA2PBA(bitmap_block_addr) ,0);
	if (b.buf == NULL)
		return -1;

	/* 将所有位初始化为零 */
	uint32_t block_size = ext4_sb_get_block_size(sb);
	uint32_t inodes_per_group = ext4_get32(sb, inodes_per_group);

	memset(b.buf->data->data, 0, (inodes_per_group + 7) / 8); // 将所有位初始化为0

	uint32_t start_bit = inodes_per_group;
	uint32_t end_bit = block_size * 8;

	uint32_t i;
	// 设置起始位到下一个8位的整数倍之间的位为1
	for (i = start_bit; i < ((start_bit + 7) & ~7UL); i++)
		ext4_bmap_bit_set(b.buf->data->data, i);

	// 如果还有剩余的位，将这些位全部设置为1
	if (i < end_bit)
		memset(b.buf->data->data + (i >> 3), 0xff, (end_bit - i) >> 3);

	ext4_ialloc_set_bitmap_csum(sb, bg, b.buf->data->data); // 计算并设置校验和
	bg_ref->dirty = true;
	bufWrite(b.buf);// 保存位图
	bufRelease(b.buf);
	return 0; 
}

/**
 * @brief 初始化块组中的i-node表
 * @param bg_ref 块组的引用
 * @return 错误代码
 */
static int ext4_fs_init_inode_table(struct ext4_block_group_ref *bg_ref)
{
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
	struct ext4_bgroup *bg = bg_ref->block_group;

	uint32_t inode_size = ext4_get16(sb, inode_size);
	uint32_t block_size = ext4_sb_get_block_size(sb);
	uint32_t inodes_per_block = block_size / inode_size;
	uint32_t inodes_in_group = ext4_inodes_in_group_cnt(sb, bg_ref->index);
	uint32_t table_blocks = inodes_in_group / inodes_per_block;
	ext4_fsblk_t fblock;

	// 如果组内的i-node数量不能被每块的i-node数量整除，则需要多一个块
	if (inodes_in_group % inodes_per_block)
		table_blocks++;

	// 计算初始化的范围
	ext4_fsblk_t first_block = ext4_bg_get_inode_table_first_block(bg, sb);
	ext4_fsblk_t last_block = first_block + table_blocks - 1;

	// 初始化所有i-node表块
	for (fblock = first_block; fblock <= last_block; ++fblock) {
		struct ext4_block b;
		b.buf = bufRead(1,EXT4_LBA2PBA(fblock) ,0);
		if (b.buf == NULL)
		{
			return -1;
		}
		// 将块的数据清零
		memset(b.buf->data->data, 0, block_size);
		// 将块标记为脏块
		bufWrite(b.buf);
		bufRelease(b.buf);
	}

	return 0; // 返回成功状态
}


int ext4_fs_get_block_group_ref(struct FileSystem *fs, uint32_t bgid,
				struct ext4_block_group_ref *ref)
{
	int rc;
	// 计算一个数据块中能容纳的描述符数量
	uint32_t block_size = ext4_sb_get_block_size(&fs->superBlock.ext4_sblock);
	uint32_t dsc_cnt = block_size / ext4_sb_get_desc_size(&fs->superBlock.ext4_sblock);

	// 块组描述符表从超级块之后的下一个块开始
	uint64_t block_id = ext4_fs_get_descriptor_block(&fs->superBlock.ext4_sblock, bgid, dsc_cnt);
	// 计算描述符在块中的偏移量
	uint32_t offset = (bgid % dsc_cnt) * ext4_sb_get_desc_size(&fs->superBlock.ext4_sblock);
	ref->block.buf = bufRead(1,EXT4_LBA2PBA(block_id),1);
	ref->block_group = (void *)(ref->block.buf->data->data + offset);
	ref->fs = &fs->ext4_fs;
	ref->index = bgid;
	ref->dirty = false;
	struct ext4_bgroup *bg = ref->block_group;
	//检查块组中块位图的使用情况
	if (ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_BLOCK_UNINIT)) {
		rc = ext4_fs_init_block_bitmap(ref);
		if (rc != 0) {
			bufRelease(ref->block.buf);
			return rc;
		}
		ext4_bg_clear_flag(bg, EXT4_BLOCK_GROUP_BLOCK_UNINIT);
		ref->dirty = true;
	}
	//检查块组中inode位图的使用情况
	if (ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_INODE_UNINIT)) {
		rc = ext4_fs_init_inode_bitmap(ref);
		if (rc != 0) {
			bufRelease(ref->block.buf);
			return rc;
		}
		ext4_bg_clear_flag(bg, EXT4_BLOCK_GROUP_INODE_UNINIT);

		if (!ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_ITABLE_ZEROED)) {
			rc = ext4_fs_init_inode_table(ref);
			if (rc != 0) {
				bufRelease(ref->block.buf);
				return rc;
			}
			ext4_bg_set_flag(bg, EXT4_BLOCK_GROUP_ITABLE_ZEROED);
		}

		ref->dirty = true;
	}

	return 0;
}

/* 从原生的 inode 中获取块索引，存在 fblock 里面*/
static int ext4_fs_get_inode_dblk_idx_internal(struct ext4_inode_ref *inode_ref, uint64_t iblock,uint64_t *fblock, bool extent_create, bool support_unwritten __attribute__ ((__unused__)))
{
	struct ext4_fs *fs = &ext4Fs->ext4_fs;
	// 对于空文件，直接返回0
	if (ext4_inode_get_size(&ext4Fs->superBlock.ext4_sblock, inode_ref->inode) == 0) {
	    *fblock = 0;
	    return 0;
	}
	uint64_t current_block;
	/* 如果使用了extent机制*/
	if ((ext4_sb_feature_incom(&ext4Fs->superBlock.ext4_sblock, EXT4_FINCOM_EXTENTS)) &&
	    (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {

		ext4_fsblk_t current_fsblk;
		int rc = ext4_extent_get_blocks(inode_ref, iblock, 1, &current_fsblk, extent_create, NULL);
		if (rc != 0)
			return rc;

		current_block = current_fsblk;
		*fblock = current_block;

		ASSERT(*fblock || support_unwritten);
		return 0;
	}
	struct ext4_inode *inode = inode_ref->inode;
	// 直接块从i节点结构中的数组读取
	if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
		current_block = ext4_inode_get_direct_block(inode, (uint32_t)iblock);
		*fblock = current_block;
		return 0;
	}

	// 确定目标块的间接级别
	unsigned int l = 0;
	unsigned int i;
	for (i = 1; i < 4; i++) {
		if (iblock < fs->inode_block_limits[i]) {
			l = i;
			break;
		}
	}

	if (l == 0)
		return -1;
	// 计算顶层的偏移
	uint32_t blk_off_in_lvl = (uint32_t)(iblock - fs->inode_block_limits[l - 1]);
	current_block = ext4_inode_get_indirect_block(inode, l - 1);
	uint32_t off_in_blk = (uint32_t)(blk_off_in_lvl / fs->inode_blocks_per_level[l - 1]);

	// 稀疏文件处理
	if (current_block == 0) {
		*fblock = 0;
		return 0;
    }

    struct ext4_block block;
	// 通过其他级别导航，直到找到块号或发现稀疏文件的空引用
	while (l > 0) {
		// 加载间接块
		block.buf = bufRead(1, EXT4_LBA2PBA(current_block) , 1);
		if (block.buf == NULL)
		{
			return -1;
		}
		// 从间接块中读取块地址
		current_block = to_le32(((uint32_t *)block.buf->data->data)[off_in_blk]);

		bufRelease(block.buf);

		// 检查是否为稀疏文件
		if (current_block == 0) {
			*fblock = 0;
			return 0;
		}
		// 跳到下一级
		l--;
		// 终止条件 - 已加载数据块地址
		if (l == 0)
			break;
		// 访问下一级
		blk_off_in_lvl %= fs->inode_blocks_per_level[l];
		off_in_blk = (uint32_t)(blk_off_in_lvl /fs->inode_blocks_per_level[l - 1]);
	}
	*fblock = current_block;
	return 0;
}

static uint32_t ext4_fs_inode_checksum(struct ext4_inode_ref *inode_ref)
{
	uint32_t checksum = 0;
	struct ext4_sblock *sb = &inode_ref->fs->superBlock.ext4_sblock;
	uint16_t inode_size = ext4_get16(sb, inode_size);

	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM)) {
		uint32_t orig_checksum;

		uint32_t ino_index = to_le32(inode_ref->index);
		uint32_t ino_gen =
		    to_le32(ext4_inode_get_generation(inode_ref->inode));

		/* 准备工作：暂时将bg checksum设置为0 */
		orig_checksum = ext4_inode_get_csum(sb, inode_ref->inode);
		ext4_inode_set_csum(sb, inode_ref->inode, 0);

		/* 首先根据 fs uuid 计算 crc32 校验和 */
		checksum =
		    ext4_crc32c(EXT4_CRC32_INIT, sb->uuid, sizeof(sb->uuid));
		/* 然后根据 inode 编号计算 crc32 校验和
		 * 和索引节点生成 */
		checksum = ext4_crc32c(checksum, &ino_index, sizeof(ino_index));
		checksum = ext4_crc32c(checksum, &ino_gen, sizeof(ino_gen));
		/* 最后计算crc32校验和
		 * 整个索引节点 */
		checksum = ext4_crc32c(checksum, inode_ref->inode, inode_size);
		ext4_inode_set_csum(sb, inode_ref->inode, orig_checksum);

		/* 如果 inode 大小不足以容纳
		 * 校验和的高16位 */
		if (inode_size == EXT4_GOOD_OLD_INODE_SIZE)
			checksum &= 0xFFFF;
	}
	return checksum;
}

void ext4_fs_set_inode_checksum(struct ext4_inode_ref *inode_ref)
{
	struct ext4_sblock *sb = &inode_ref->fs->superBlock.ext4_sblock;
	if (!ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM))
		return;

	uint32_t csum = ext4_fs_inode_checksum(inode_ref);
	ext4_inode_set_csum(sb, inode_ref->inode, csum);
}

int ext4_fs_get_inode_dblk_idx(struct ext4_inode_ref *inode_ref, uint64_t iblock, uint64_t *fblock, bool support_unwritten)
{
	return ext4_fs_get_inode_dblk_idx_internal(inode_ref, iblock, fblock,
						   false, support_unwritten);
}

int ext4_fs_init_inode_dblk_idx(struct ext4_inode_ref *inode_ref,
				ext4_lblk_t iblock, ext4_fsblk_t *fblock)
{
	return ext4_fs_get_inode_dblk_idx_internal(inode_ref, iblock, fblock,
						   true, true);
}

int ext4_fs_indirect_find_goal(struct ext4_inode_ref *inode_ref, ext4_fsblk_t *goal)
{
	int r;
	int EOK = 0;
	struct ext4_sblock *sb = &inode_ref->fs->superBlock.ext4_sblock;
	*goal = 0; // 初始化目标块为0

	uint64_t inode_size = ext4_inode_get_size(sb, inode_ref->inode);
	uint32_t block_size = ext4_sb_get_block_size(sb);
	uint32_t iblock_cnt = (uint32_t)(inode_size / block_size);

	// 如果inode的大小不是块大小的整数倍，则增加一个块计数
	if (inode_size % block_size != 0)
		iblock_cnt++;

	/* 如果inode有一些块，获取最后一个块的地址并增加1 */
	if (iblock_cnt > 0) {
		r = ext4_fs_get_inode_dblk_idx(inode_ref, iblock_cnt - 1, goal, false);
		if (r != EOK)
			return r;

		if (*goal != 0) {
			(*goal)++; // 如果goal不是0，增加1并返回
			return r;
		}

		/* 如果goal == 0，表示稀疏文件，继续处理 */
	}

	/* 确定inode所在的块组 */
	uint32_t inodes_per_bg = ext4_get32(sb, inodes_per_group);
	uint32_t block_group = (inode_ref->index - 1) / inodes_per_bg;
	block_size = ext4_sb_get_block_size(sb);

	/* 加载块组引用 */
	struct ext4_block_group_ref bg_ref;
	r = ext4_fs_get_block_group_ref(inode_ref->fs, block_group, &bg_ref);
	if (r != EOK)
		return r;

	struct ext4_bgroup *bg = bg_ref.block_group;

	/* 计算索引 */
	uint32_t bg_count = ext4_block_group_cnt(sb);
	ext4_fsblk_t itab_first_block = ext4_bg_get_inode_table_first_block(bg, sb);
	uint16_t itab_item_size = ext4_get16(sb, inode_size);
	uint32_t itab_bytes;

	/* 检查是否为最后一个块组 */
	if (block_group < bg_count - 1) {
		itab_bytes = inodes_per_bg * itab_item_size;
	} else {
		/* 最后一个块组可能更小 */
		uint32_t inodes_cnt = ext4_get32(sb, inodes_count);

		itab_bytes = (inodes_cnt - ((bg_count - 1) * inodes_per_bg));
		itab_bytes *= itab_item_size;
	}

	ext4_fsblk_t inode_table_blocks = itab_bytes / block_size;

	if (itab_bytes % block_size)
		inode_table_blocks++;

	*goal = itab_first_block + inode_table_blocks; // 设置目标块为inode表的最后一个块

	return ext4_fs_put_block_group_ref(&bg_ref); // 释放块组引用并返回结果
}

static int ext4_fs_set_inode_data_block_index(struct ext4_inode_ref *inode_ref,
					      ext4_lblk_t iblock,
					      ext4_fsblk_t fblock)
{
	struct ext4_fs *fs = &inode_ref->fs->ext4_fs;
	struct ext4_sblock * sb = &inode_ref->fs->superBlock.ext4_sblock;
	int EOK = 0;

	/* 处理使用extent的inode */
	if ((ext4_sb_feature_incom(sb, EXT4_FINCOM_EXTENTS)) &&
	    (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {
		/* 不可达 */
		return -1;
	}


	/* 处理直接引用的简单情况 */
	if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
		ext4_inode_set_direct_block(inode_ref->inode, (uint32_t)iblock, (uint32_t)fblock);
		inode_ref->dirty = true;

		return EOK;
	}

	/* 确定需要的间接级别 */
	unsigned int l = 0;
	unsigned int i;
	for (i = 1; i < 4; i++) {
		if (iblock < fs->inode_block_limits[i]) {
			l = i;
			break;
		}
	}

	if (l == 0)
		return -1;

	uint32_t block_size = ext4_sb_get_block_size(sb);

	/* 计算顶层的偏移量 */
	uint32_t blk_off_in_lvl = (uint32_t)(iblock - fs->inode_block_limits[l - 1]);
	ext4_fsblk_t current_block = ext4_inode_get_indirect_block(inode_ref->inode, l - 1);
	uint32_t off_in_blk = (uint32_t)(blk_off_in_lvl / fs->inode_blocks_per_level[l - 1]);

	ext4_fsblk_t new_blk;

	struct ext4_block block;
	struct ext4_block new_block;

	/* 如果在inode级别需要分配间接块 */
	if (current_block == 0) {
		/* 分配新的间接块 */
		ext4_fsblk_t goal;
		int rc = ext4_fs_indirect_find_goal(inode_ref, &goal);
		if (rc != EOK)
			return rc;

		rc = ext4_balloc_alloc_block(inode_ref, goal, &new_blk);
		if (rc != EOK)
			return rc;

		/* 更新inode */
		ext4_inode_set_indirect_block(inode_ref->inode, l - 1,(uint32_t)new_blk);
		inode_ref->dirty = true;

		/* 加载新分配的块 */
		new_block.buf = bufRead(1,EXT4_LBA2PBA(new_blk) ,0);//直接从内存加载
		//rc = ext4_trans_block_get_noread(fs->bdev, &new_block, new_blk);
		if (new_block.buf == NULL) {
			ext4_balloc_free_blocks(inode_ref, new_blk,1);
			return rc;
		}

		/* 初始化新块 */
		memset(new_block.buf->data->data, 0, block_size);
		bufWrite(new_block.buf);//标记为脏页
		
		/* 释放分配的块 */
		bufRelease(new_block.buf);
		
		current_block = new_blk;
	}

	/*
	 * 通过其他级别，直到找到块号或找到空引用，表示稀疏文件
	 */
	while (l > 0) {
		block.buf = bufRead(1,EXT4_LBA2PBA(current_block),1);
		if (block.buf == NULL)
			return -1;

		current_block = to_le32(((uint32_t *)block.buf->data->data)[off_in_blk]);
		if ((l > 1) && (current_block == 0)) {
			ext4_fsblk_t goal;
			int rc = ext4_fs_indirect_find_goal(inode_ref, &goal);
			if (rc != EOK) {
				bufRelease(block.buf);
				return rc;
			}
			/* 分配新块 */
			rc = ext4_balloc_alloc_block(inode_ref, goal, &new_blk);
			if (rc != EOK) {
				bufRelease(block.buf);
				return rc;
			}

			/* 加载新分配的块 */
			new_block.buf = bufRead(1, EXT4_LBA2PBA(new_blk),0);//直接从内存加载

			if (rc != EOK) {
				bufRelease(block.buf);
				return rc;
			}

			/* 初始化分配的块 */
			memset(new_block.buf->data->data, 0, block_size);
			bufWrite(new_block.buf);//标记为脏页

			if (rc != EOK) {
				bufRelease(block.buf);
				return rc;
			}

			/* 将块地址写入父块 */
			uint32_t *p = (uint32_t *)block.buf->data->data;
			p[off_in_blk] = to_le32((uint32_t)new_blk);
			ext4_trans_set_block_dirty(block.buf);
			current_block = new_blk;
		}

		/* 在最后一级，写入fblock地址 */
		if (l == 1) {
			uint32_t *p = (uint32_t *)block.buf->data->data
			;
			p[off_in_blk] = to_le32((uint32_t)fblock);
			ext4_trans_set_block_dirty(block.buf);
		}
		//回收资源
		bufRelease(block.buf);

		l--;

		/*
		 * 如果在最后一级，直接退出，因为没有下一级
		 */
		if (l == 0)
			break;

		/* 访问下一级 */
		blk_off_in_lvl %= fs->inode_blocks_per_level[l];
		off_in_blk = (uint32_t)(blk_off_in_lvl / fs->inode_blocks_per_level[l - 1]);
	}

	return EOK;
}

int ext4_fs_append_inode_dblk(struct ext4_inode_ref *inode_ref, ext4_fsblk_t *fblock, ext4_lblk_t *iblock)
{
	if ((ext4_sb_feature_incom(&ext4Fs->superBlock.ext4_sblock, EXT4_FINCOM_EXTENTS)) &&
	    (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {
		
		int rc;
		ext4_fsblk_t current_fsblk;
		struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
		uint64_t inode_size = ext4_inode_get_size(sb, inode_ref->inode);
		uint32_t block_size = ext4_sb_get_block_size(sb);
		*iblock =
		    (uint32_t)((inode_size + block_size - 1) / block_size);

		rc = ext4_extent_get_blocks(inode_ref, *iblock, 1,
					    &current_fsblk, true, NULL);
		if (rc != 0)
			return rc;

		*fblock = current_fsblk;
		ASSERT(*fblock);
		ext4_inode_set_size(inode_ref->inode, inode_size + block_size);
		inode_ref->dirty = true;

		return rc;
	}
	struct ext4_sblock *sb = &inode_ref->fs->superBlock.ext4_sblock;

	/* 计算下一个块索引并分配数据块 */
	uint64_t inode_size = ext4_inode_get_size(sb, inode_ref->inode);
	uint32_t block_size = ext4_sb_get_block_size(sb);
	/* 对齐inode的大小 */
	if ((inode_size % block_size) != 0)
		inode_size += block_size - (inode_size % block_size);

	/* 逻辑块从0开始编号 */
	uint32_t new_block_idx = (uint32_t)(inode_size / block_size);

	/* 分配新的物理块 */
	ext4_fsblk_t goal, phys_block;
	int rc = ext4_fs_indirect_find_goal(inode_ref, &goal);
	if (rc != 0)
		return rc;

	rc = ext4_balloc_alloc_block(inode_ref, goal, &phys_block);
	if (rc != 0)
		return rc;

	/* 将物理块地址添加到inode */
	rc = ext4_fs_set_inode_data_block_index(inode_ref, new_block_idx,phys_block);
	if (rc != 0) {
		ext4_balloc_free_blocks(inode_ref, phys_block,1);
		return rc;
	}

	/* 更新inode */
	ext4_inode_set_size(inode_ref->inode, inode_size + block_size);
	inode_ref->dirty = true;

	*fblock = phys_block;
	*iblock = new_block_idx;

	return 0;
}


static void ext4_fs_debug_features_inc(uint32_t features_incompatible)
{
	if (features_incompatible & EXT4_FINCOM_COMPRESSION)
		printk("compression\n");
	if (features_incompatible & EXT4_FINCOM_FILETYPE)
		printk("filetype\n");
	if (features_incompatible & EXT4_FINCOM_RECOVER)
		printk("recover\n");
	if (features_incompatible & EXT4_FINCOM_JOURNAL_DEV)
		printk("journal_dev\n");
	if (features_incompatible & EXT4_FINCOM_META_BG)
		printk("meta_bg\n");
	if (features_incompatible & EXT4_FINCOM_EXTENTS)
		printk("extents\n");
	if (features_incompatible & EXT4_FINCOM_64BIT)
		printk("64bit\n");
	if (features_incompatible & EXT4_FINCOM_MMP)
		printk("mnp\n");
	if (features_incompatible & EXT4_FINCOM_FLEX_BG)
		printk("flex_bg\n");
	if (features_incompatible & EXT4_FINCOM_EA_INODE)
		printk("ea_inode\n");
	if (features_incompatible & EXT4_FINCOM_DIRDATA)
		printk("dirdata\n");
	if (features_incompatible & EXT4_FINCOM_BG_USE_META_CSUM)
		printk("meta_csum\n");
	if (features_incompatible & EXT4_FINCOM_LARGEDIR)
		printk("largedir\n");
	if (features_incompatible & EXT4_FINCOM_INLINE_DATA)
		printk("inline_data\n");
}
static void ext4_fs_debug_features_comp(uint32_t features_compatible)
{
	if (features_compatible & EXT4_FCOM_DIR_PREALLOC)
		printk("dir_prealloc\n");
	if (features_compatible & EXT4_FCOM_IMAGIC_INODES)
		printk("imagic_inodes\n");
	if (features_compatible & EXT4_FCOM_HAS_JOURNAL)
		printk("has_journal\n");
	if (features_compatible & EXT4_FCOM_EXT_ATTR)
		printk("ext_attr\n");
	if (features_compatible & EXT4_FCOM_RESIZE_INODE)
		printk("resize_inode\n");
	if (features_compatible & EXT4_FCOM_DIR_INDEX)
		printk("dir_index\n");
}

static void ext4_fs_debug_features_ro(uint32_t features_ro)
{
	if (features_ro & EXT4_FRO_COM_SPARSE_SUPER)
		printk("sparse_super\n");
	if (features_ro & EXT4_FRO_COM_LARGE_FILE)
		printk("large_file\n");
	if (features_ro & EXT4_FRO_COM_BTREE_DIR)
		printk("btree_dir\n");
	if (features_ro & EXT4_FRO_COM_HUGE_FILE)
		printk("huge_file\n");
	if (features_ro & EXT4_FRO_COM_GDT_CSUM)
		printk("gtd_csum\n");
	if (features_ro & EXT4_FRO_COM_DIR_NLINK)
		printk("dir_nlink\n");
	if (features_ro & EXT4_FRO_COM_EXTRA_ISIZE)
		printk("extra_isize\n");
	if (features_ro & EXT4_FRO_COM_QUOTA)
		printk("quota\n");
	if (features_ro & EXT4_FRO_COM_BIGALLOC)
		printk("bigalloc\n");
	if (features_ro & EXT4_FRO_COM_METADATA_CSUM)
		printk("metadata_csum\n");
}
int ext4_fs_check_features(struct ext4_fs *fs)
{
	uint32_t v;
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
	if (ext4_get32(sb, rev_level) == 0) {
		return 1;
	}

	printk("sblock features_incompatible:\n");
	ext4_fs_debug_features_inc(ext4_get32(sb, features_incompatible));

	printk("sblock features_compatible:\n");
	ext4_fs_debug_features_comp(ext4_get32(sb, features_compatible));

	printk("sblock features_read_only:\n");
	ext4_fs_debug_features_ro(ext4_get32(sb, features_read_only));

	/*检查功能不兼容*/
	v = (ext4_get32(sb, features_incompatible) & (~CONFIG_SUPPORTED_FINCOM));
	
	if (v) {
		printk("sblock has unsupported features incompatible:\n");
		ext4_fs_debug_features_inc(v);
		return -1;
	}
	
	/*检查 features_read_only*/
	v = ext4_get32(sb, features_read_only);
	v &= ~CONFIG_SUPPORTED_FRO_COM;
	if (v) {
		printk("sblock has unsupported features read only:\n");
		ext4_fs_debug_features_ro(v);
		return 1;
	}
	printk("ext4_fs_check_features\n");
	return 1;
}

uint32_t ext4_fs_correspond_inode_mode(int filetype)
{
	switch (filetype) {
	case EXT4_DE_DIR:
		return EXT4_INODE_MODE_DIRECTORY;
	case EXT4_DE_REG_FILE:
		return EXT4_INODE_MODE_FILE;
	case EXT4_DE_SYMLINK:
		return EXT4_INODE_MODE_SOFTLINK;
	case EXT4_DE_CHRDEV:
		return EXT4_INODE_MODE_CHARDEV;
	case EXT4_DE_BLKDEV:
		return EXT4_INODE_MODE_BLOCKDEV;
	case EXT4_DE_FIFO:
		return EXT4_INODE_MODE_FIFO;
	case EXT4_DE_SOCK:
		return EXT4_INODE_MODE_SOCKET;
	}
	/* FIXME: unsupported filetype */
	return EXT4_INODE_MODE_FILE;
}
