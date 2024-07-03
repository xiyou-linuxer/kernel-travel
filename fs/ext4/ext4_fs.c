#include <fs/ext4.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_block_group.h>
#include <fs/ext4_sb.h>
#include <fs/buf.h>
#include <fs/fs.h>
#include <xkernel/stdio.h>

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

/**@brief 初始化块组中的块位图
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
	rc = ext4_trans_block_get_noread(bg_ref->fs->bdev, &block_bitmap, bmp_blk);
	if (rc != EOK)
		return rc;

	// 初始化块位图的内容为0
	memset(block_bitmap.data, 0, block_size);
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
		ext4_bmap_bit_set(block_bitmap.data, bit);

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
	ext4_balloc_set_bitmap_csum(sb, bg_ref->block_group, block_bitmap.data);
	bg_ref->dirty = true;

	// 保存位图
	return ext4_block_set(bg_ref->fs->bdev, &block_bitmap);
}


int ext4_fs_get_block_group_ref(struct FileSystem *fs, uint32_t bgid,
				struct ext4_block_group_ref *ref)
{
	int rc;
	/* Compute number of descriptors, that fits in one data block */
	uint32_t block_size = ext4_sb_get_block_size(&fs->superBlock);
	uint32_t dsc_cnt = block_size / ext4_sb_get_desc_size(&fs->superBlock);

	/* Block group descriptor table starts at the next block after
	 * superblock */
	uint64_t block_id = ext4_fs_get_descriptor_block(&fs->superBlock, bgid, dsc_cnt);

	uint32_t offset = (bgid % dsc_cnt) * ext4_sb_get_desc_size(&fs->superBlock);
	ref->block.buf = bufRead(1,block_id,1);
	
	ref->block_group = (void *)(ref->block.buf->data + offset);
	ref->fs = fs;
	ref->index = bgid;
	ref->dirty = false;
	struct ext4_bgroup *bg = ref->block_group;

	if (ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_BLOCK_UNINIT)) {
		rc = ext4_fs_init_block_bitmap(ref);
		if (rc != 0) {
			ext4_block_set(fs->bdev, &ref->block);
			return rc;
		}
		ext4_bg_clear_flag(bg, EXT4_BLOCK_GROUP_BLOCK_UNINIT);
		ref->dirty = true;
	}

	if (ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_INODE_UNINIT)) {
		rc = ext4_fs_init_inode_bitmap(ref);
		if (rc != 0) {
			ext4_block_set(ref->fs->bdev, &ref->block);
			return rc;
		}

		ext4_bg_clear_flag(bg, EXT4_BLOCK_GROUP_INODE_UNINIT);

		if (!ext4_bg_has_flag(bg, EXT4_BLOCK_GROUP_ITABLE_ZEROED)) {
			rc = ext4_fs_init_inode_table(ref);
			if (rc != 0) {
				ext4_block_set(fs->bdev, &ref->block);
				return rc;
			}

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

    // 如果启用了扩展支持并且i节点使用扩展方式存储块
#if CONFIG_EXTENT_ENABLE && CONFIG_EXTENTS_ENABLE
    if ((ext4_sb_feature_incom(&fs->sb, EXT4_FINCOM_EXTENTS)) &&
        (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {

        uint64_t current_fsblk;
        int rc = ext4_extent_get_blocks(
            inode_ref, iblock, 1, &current_fsblk, extent_create, NULL);
        if (rc != EOK)
            return rc;

        current_block = current_fsblk;
        *fblock = current_block;

        ext4_assert(*fblock || support_unwritten);
        return EOK;
    }
#endif

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
