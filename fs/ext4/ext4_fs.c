#include <fs/ext4.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_block_group.h>
#include <fs/ext4_sb.h>
#include <fs/ext4_crc32.h>
#include <fs/ext4_bitmap.h>
#include <fs/buf.h>
#include <fs/fs.h>
#include <xkernel/stdio.h>

int ext4_fs_put_block_group_ref(struct ext4_block_group_ref *ref)
{
    // 检查引用是否被修改
    if (ref->dirty) {
        uint16_t cs;

        // 计算块组的新校验和
        cs = ext4_fs_bg_checksum(&ref->fs->sb, ref->index, ref->block_group);
        ref->block_group->checksum = to_le16(cs);

        // 标记块为脏块以便将更改写入物理设备
        bufWrite(ref->block.buf);
        ext4_trans_set_block_dirty(ref->block.buf);
    }
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
	struct ext4_sblock *sb = &bg_ref->fs->sb;
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
	bufRead(1,bmp_blk,0);
	// 初始化块位图的内容为0
	memset(block_bitmap.buf->data, 0, block_size);
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
		ext4_bmap_bit_set(block_bitmap.buf->data, bit);

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
		ext4_bmap_bit_set(block_bitmap.buf->data, (uint32_t)(bmp_blk - first_bg));

	in_bg = ext4_block_in_group(sb, bmp_inode, bg_ref->index);
	if (!flex_bg || in_bg)
		ext4_bmap_bit_set(block_bitmap.buf->data, (uint32_t)(bmp_inode - first_bg));

	// 设置 inode 表中所有块在块位图中的对应位为 1
	for (i = inode_table; i < inode_table + inode_table_bcnt; i++) {
		in_bg = ext4_block_in_group(sb, i, bg_ref->index);
		if (!flex_bg || in_bg)
			ext4_bmap_bit_set(block_bitmap.buf->data, (uint32_t)(i - first_bg));
	}

	/*
	 * 如果组内的块数小于块大小 * 8（这是位图的大小），则将块位图的其余部分设置为1
	 */
	ext4_fs_mark_bitmap_end(group_blocks, block_size * 8, block_bitmap.buf->data);
	ext4_trans_set_block_dirty(block_bitmap.buf);

	// 设置块组的位图校验和
	ext4_balloc_set_bitmap_csum(sb, bg_ref->block_group, block_bitmap.buf->data);
	bg_ref->dirty = true;

	// 保存位图
	bufWrite(block_bitmap.buf);
	return 0;
}

void ext4_ialloc_set_bitmap_csum(struct ext4_sblock *sb, struct ext4_bgroup *bg,
				 void *bitmap __attribute__ ((__unused__)))
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
	struct ext4_sblock *sb = &bg_ref->fs->sb;
	struct ext4_bgroup *bg = bg_ref->block_group;

	/* 获取 inode 位图所在的磁盘扇区号 */
	ext4_fsblk_t bitmap_block_addr = ext4_bg_get_inode_bitmap(bg, sb);

	struct ext4_block b;
	/*读取*/
	b.buf = bufRead(1,bitmap_block_addr,1);
	if (b.buf == NULL)
		return -1;

	/* 将所有位初始化为零 */
	uint32_t block_size = ext4_sb_get_block_size(sb);
	uint32_t inodes_per_group = ext4_get32(sb, inodes_per_group);

	memset(b.buf->data, 0, (inodes_per_group + 7) / 8); // 将所有位初始化为0

	uint32_t start_bit = inodes_per_group;
	uint32_t end_bit = block_size * 8;

	uint32_t i;
	// 设置起始位到下一个8位的整数倍之间的位为1
	for (i = start_bit; i < ((start_bit + 7) & ~7UL); i++)
		ext4_bmap_bit_set(b.buf->data, i);

	// 如果还有剩余的位，将这些位全部设置为1
	if (i < end_bit)
		memset(b.buf->data + (i >> 3), 0xff, (end_bit - i) >> 3);

	ext4_ialloc_set_bitmap_csum(sb, bg, b.buf->data); // 计算并设置校验和
	bg_ref->dirty = true;
	bufWrite(b.buf);// 保存位图
	return 0; 
}

/**
 * @brief 初始化块组中的i-node表
 * @param bg_ref 块组的引用
 * @return 错误代码
 */
static int ext4_fs_init_inode_table(struct ext4_block_group_ref *bg_ref)
{
	struct ext4_sblock *sb = &bg_ref->fs->sb;
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
		b.buf = bufRead(1,fblock,1);
		// 将块的数据清零
		memset(b.buf->data, 0, block_size);
		// 将块标记为脏块
		bufWrite(b.buf);
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
	ref->block.buf = bufRead(1,block_id,1);
	
	ref->block_group = (void *)(ref->block.buf->data + offset);
	ref->fs = fs;
	ref->index = bgid;
	ref->dirty = false;
	struct ext4_bgroup *bg = ref->block_group;
	//检查块组中块位图的使用情况
	if (ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_BLOCK_UNINIT)) {
		rc = ext4_fs_init_block_bitmap(ref);
		ext4_bg_clear_flag(bg, EXT4_BLOCK_GROUP_BLOCK_UNINIT);
		ref->dirty = true;
	}
	//检查块组中inode位图的使用情况
	if (ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_INODE_UNINIT)) {
		rc = ext4_fs_init_inode_bitmap(ref);
		ext4_bg_clear_flag(bg, EXT4_BLOCK_GROUP_INODE_UNINIT);

		if (!ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_ITABLE_ZEROED)) {
			rc = ext4_fs_init_inode_table(ref);
			ext4_bg_set_flag(bg, EXT4_BLOCK_GROUP_ITABLE_ZEROED);
		}

		ref->dirty = true;
	}

	return 0;
}

static int ext4_fs_get_inode_dblk_idx_internal(struct ext4_inode_ref *inode_ref,uint64_t iblock,uint64_t *fblock,bool extent_create,
bool support_unwritten __attribute__ ((__unused__)))
{
    struct FileSystem *fs = inode_ref->fs;

    // 对于空文件，直接返回0
    if (ext4_inode_get_size(&fs->superBlock.ext4_sblock, inode_ref->inode) == 0) {
        *fblock = 0;
        return 0;
    }

    uint64_t current_block;
    struct ext4_inode *inode = inode_ref->inode;

    // 直接块从i节点结构中的数组读取
    if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
        current_block =
            ext4_inode_get_direct_block(inode, (uint32_t)iblock);
        *fblock = current_block;
        return 0;
    }

    // 确定目标块的间接级别
    unsigned int l = 0;
    unsigned int i;
    for (i = 1; i < 4; i++) {
        if (iblock < fs->superBlock.ext4_sblock.inode_block_limits[i]) {
            l = i;
            break;
        }
    }

    if (l == 0)
        return -1;

    // 计算顶层的偏移
    uint32_t blk_off_in_lvl =
        (uint32_t)(iblock - fs->inode_block_limits[l - 1]);
    current_block = ext4_inode_get_indirect_block(inode, l - 1);
    uint32_t off_in_blk =
        (uint32_t)(blk_off_in_lvl / fs->inode_blocks_per_level[l - 1]);

    // 稀疏文件处理
    if (current_block == 0) {
        *fblock = 0;
        return 0;
    }

    struct ext4_block block;

    // 通过其他级别导航，直到找到块号或发现稀疏文件的空引用
    while (l > 0) {
        // 加载间接块
        int rc = ext4_trans_block_get(fs->bdev, &block, current_block);
        if (rc != 0)
            return rc;

        // 从间接块中读取块地址
        current_block = to_le32(((uint32_t *)block.data)[off_in_blk]);

        // 未修改的间接块放回
        rc = ext4_block_set(fs->bdev, &block);
        if (rc != 0)
            return rc;

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

		/* Preparation: temporarily set bg checksum to 0 */
		orig_checksum = ext4_inode_get_csum(sb, inode_ref->inode);
		ext4_inode_set_csum(sb, inode_ref->inode, 0);

		/* First calculate crc32 checksum against fs uuid */
		checksum =
		    ext4_crc32c(EXT4_CRC32_INIT, sb->uuid, sizeof(sb->uuid));
		/* Then calculate crc32 checksum against inode number
		 * and inode generation */
		checksum = ext4_crc32c(checksum, &ino_index, sizeof(ino_index));
		checksum = ext4_crc32c(checksum, &ino_gen, sizeof(ino_gen));
		/* Finally calculate crc32 checksum against
		 * the entire inode */
		checksum = ext4_crc32c(checksum, inode_ref->inode, inode_size);
		ext4_inode_set_csum(sb, inode_ref->inode, orig_checksum);

		/* If inode size is not large enough to hold the
		 * upper 16bit of the checksum */
		if (inode_size == EXT4_GOOD_OLD_INODE_SIZE)
			checksum &= 0xFFFF;
	}
	return checksum;
}

static void ext4_fs_set_inode_checksum(struct ext4_inode_ref *inode_ref)
{
	struct ext4_sblock *sb = &inode_ref->fs->superBlock;
	if (!ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM))
		return;

	uint32_t csum = ext4_fs_inode_checksum(inode_ref);
	ext4_inode_set_csum(sb, inode_ref->inode, csum);
}
/**
 * 
*/
int ext4_fs_get_inode_dblk_idx(struct ext4_inode_ref *inode_ref,
			       uint64_t iblock, uint64_t *fblock,
			       bool support_unwritten)
{
	return ext4_fs_get_inode_dblk_idx_internal(inode_ref, iblock, fblock,
						   false, support_unwritten);
}

