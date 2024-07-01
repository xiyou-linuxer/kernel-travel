#include <fs/ext4.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/buf.h>
#include <xkernel/stdio.h>

static int ext4_fs_get_inode_dblk_idx_internal(struct ext4_inode_ref *inode_ref,uint64_t iblock,uint64_t *fblock,bool extent_create,
bool support_unwritten __attribute__ ((__unused__)))
{
    struct ext4_fs *fs = inode_ref->fs;

    // 对于空文件，直接返回0
    if (ext4_inode_get_size(&fs->sb, inode_ref->inode) == 0) {
        *fblock = 0;
        return EOK;
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
        return EOK;
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
        return EIO;

    // 计算顶层的偏移
    uint32_t blk_off_in_lvl =
        (uint32_t)(iblock - fs->inode_block_limits[l - 1]);
    current_block = ext4_inode_get_indirect_block(inode, l - 1);
    uint32_t off_in_blk =
        (uint32_t)(blk_off_in_lvl / fs->inode_blocks_per_level[l - 1]);

    // 稀疏文件处理
    if (current_block == 0) {
        *fblock = 0;
        return EOK;
    }

    struct ext4_block block;

    // 通过其他级别导航，直到找到块号或发现稀疏文件的空引用
    while (l > 0) {
        // 加载间接块
        int rc = ext4_trans_block_get(fs->bdev, &block, current_block);
        if (rc != EOK)
            return rc;

        // 从间接块中读取块地址
        current_block = to_le32(((uint32_t *)block.data)[off_in_blk]);

        // 未修改的间接块放回
        rc = ext4_block_set(fs->bdev, &block);
        if (rc != EOK)
            return rc;

        // 检查是否为稀疏文件
        if (current_block == 0) {
            *fblock = 0;
            return EOK;
        }

        // 跳到下一级
        l--;

        // 终止条件 - 已加载数据块地址
        if (l == 0)
            break;

        // 访问下一级
        blk_off_in_lvl %= fs->inode_blocks_per_level[l];
        off_in_blk = (uint32_t)(blk_off_in_lvl /
                                fs->inode_blocks_per_level[l - 1]);
    }

    *fblock = current_block;

    return EOK;
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