#ifndef EXT4_BLOCK_GROUP_H_
#define EXT4_BLOCK_GROUP_H_

#include <fs/ext4_sb.h>
#include <fs/ext4.h>
#include <fs/ext4_fs.h>
#include <xkernel/types.h>
/**
 * @brief Get address of block with data block bitmap.
 * @param bg pointer to block group
 * @param s pointer to superblock
 * @return Address of block with block bitmap
 */
static inline uint64_t ext4_bg_get_block_bitmap(struct ext4_bgroup *bg,
						struct ext4_sblock *s)
{
	uint64_t v = to_le32(bg->block_bitmap_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint64_t)to_le32(bg->block_bitmap_hi) << 32;

	return v;
}

/**
 * @brief Set address of block with data block bitmap.
 * @param bg pointer to block group
 * @param s pointer to superblock
 * @param blk block to set
 */
static inline void ext4_bg_set_block_bitmap(struct ext4_bgroup *bg,
					    struct ext4_sblock *s, uint64_t blk)
{

	bg->block_bitmap_lo = to_le32((uint32_t)blk);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->block_bitmap_hi = to_le32(blk >> 32);

}

/**
 * @brief Get address of block with i-node bitmap.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @return Address of block with i-node bitmap
 */
static inline uint64_t ext4_bg_get_inode_bitmap(struct ext4_bgroup *bg,
						struct ext4_sblock *s)
{

	uint64_t v = to_le32(bg->inode_bitmap_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint64_t)to_le32(bg->inode_bitmap_hi) << 32;

	return v;
}

/**
 * @brief Set address of block with i-node bitmap.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @param blk block to set
 */
static inline void ext4_bg_set_inode_bitmap(struct ext4_bgroup *bg,
					    struct ext4_sblock *s, uint64_t blk)
{
	bg->inode_bitmap_lo = to_le32((uint32_t)blk);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->inode_bitmap_hi = to_le32(blk >> 32);

}


/**
 * @brief Get address of the first block of the i-node table.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @return Address of first block of i-node table
 */
static inline uint64_t
ext4_bg_get_inode_table_first_block(struct ext4_bgroup *bg,
				    struct ext4_sblock *s)
{
	uint64_t v = to_le32(bg->inode_table_first_block_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint64_t)to_le32(bg->inode_table_first_block_hi) << 32;

	return v;
}

/**
 * @brief Set address of the first block of the i-node table.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @param blk block to set
 */
static inline void
ext4_bg_set_inode_table_first_block(struct ext4_bgroup *bg,
				    struct ext4_sblock *s, uint64_t blk)
{
	bg->inode_table_first_block_lo = to_le32((uint32_t)blk);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->inode_table_first_block_hi = to_le32(blk >> 32);
}

/**
 * @brief Get number of free blocks in block group.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @return Number of free blocks in block group
 */
static inline uint32_t ext4_bg_get_free_blocks_count(struct ext4_bgroup *bg,
						     struct ext4_sblock *s)
{
	uint32_t v = to_le16(bg->free_blocks_count_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->free_blocks_count_hi) << 16;

	return v;
}

/**
 * @brief Set number of free blocks in block group.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @param cnt Number of free blocks in block group
 */
static inline void ext4_bg_set_free_blocks_count(struct ext4_bgroup *bg,
						 struct ext4_sblock *s,
						 uint32_t cnt)
{
	bg->free_blocks_count_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->free_blocks_count_hi = to_le16(cnt >> 16);
}

/**
 * @brief Get number of free i-nodes in block group.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @return Number of free i-nodes in block group
 */
static inline uint32_t ext4_bg_get_free_inodes_count(struct ext4_bgroup *bg,
						     struct ext4_sblock *s)
{
	uint32_t v = to_le16(bg->free_inodes_count_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->free_inodes_count_hi) << 16;

	return v;
}

/**
 * @brief Set number of free i-nodes in block group.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @param cnt Number of free i-nodes in block group
 */
static inline void ext4_bg_set_free_inodes_count(struct ext4_bgroup *bg,
						 struct ext4_sblock *s,
						 uint32_t cnt)
{
	bg->free_inodes_count_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->free_inodes_count_hi = to_le16(cnt >> 16);
}

/**
 * @brief Get number of used directories in block group.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @return Number of used directories in block group
 */
static inline uint32_t ext4_bg_get_used_dirs_count(struct ext4_bgroup *bg,
						   struct ext4_sblock *s)
{
	uint32_t v = to_le16(bg->used_dirs_count_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->used_dirs_count_hi) << 16;

	return v;
}

/**
 * @brief Set number of used directories in block group.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @param cnt Number of used directories in block group
 */
static inline void ext4_bg_set_used_dirs_count(struct ext4_bgroup *bg,
					       struct ext4_sblock *s,
					       uint32_t cnt)
{
	bg->used_dirs_count_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->used_dirs_count_hi = to_le16(cnt >> 16);
}

/**
 * @brief Get number of unused i-nodes.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @return Number of unused i-nodes
 */
static inline uint32_t ext4_bg_get_itable_unused(struct ext4_bgroup *bg,
						 struct ext4_sblock *s)
{

	uint32_t v = to_le16(bg->itable_unused_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->itable_unused_hi) << 16;

	return v;
}

/**
 * @brief Set number of unused i-nodes.
 * @param bg Pointer to block group
 * @param s Pointer to superblock
 * @param cnt Number of unused i-nodes
 */
static inline void ext4_bg_set_itable_unused(struct ext4_bgroup *bg,
					     struct ext4_sblock *s,
					     uint32_t cnt)
{
	bg->itable_unused_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->itable_unused_hi = to_le16(cnt >> 16);
}

/**
 * @brief  Set checksum of block group.
 * @param bg Pointer to block group
 * @param crc Cheksum of block group
 */
static inline void ext4_bg_set_checksum(struct ext4_bgroup *bg, uint16_t crc)
{
	bg->checksum = to_le16(crc);
}

/**
 * @brief 检查块组是否有标志。
 * @param bg 指向块组的指针
 * @param f 要检查的标志
 * @return True 如果标志设置为 1
 */
static inline bool ext4_bg_has_flag(struct ext4_bgroup *bg, uint32_t f)
{
	return to_le16(bg->flags) & f;
}

/**
 * @brief Set flag of block group.
 * @param bg Pointer to block group
 * @param f Flag to be set
 */
static inline void ext4_bg_set_flag(struct ext4_bgroup *bg, uint32_t f)
{
	uint16_t flags = to_le16(bg->flags);
	flags |= f;
	bg->flags = to_le16(flags);
}

/**
 * @brief Clear flag of block group.
 * @param bg Pointer to block group
 * @param f Flag to be cleared
 */
static inline void ext4_bg_clear_flag(struct ext4_bgroup *bg, uint32_t f)
{
	uint16_t flags = to_le16(bg->flags);
	flags &= ~f;
	bg->flags = to_le16(flags);
}

/**@brief Calculate CRC16 of the block group.
 * @param crc Init value
 * @param buffer Input buffer
 * @param len Sizeof input buffer
 * @return Computed CRC16*/
uint16_t ext4_bg_crc16(uint16_t crc, const uint8_t *buffer, size_t len);



#endif /* EXT4_BLOCK_GROUP_H_ */
