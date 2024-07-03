#ifndef EXT4_SUPER_H_
#define EXT4_SUPER_H_

#include <xkernel/types.h>
#include <fs/ext4.h>

/**
 * @brief   Blocks count get stored in superblock.
 * @param   s superblock descriptor
 * @return  count of blocks
 **/
static inline uint64_t ext4_sb_get_blocks_cnt(struct ext4_sblock *s)
{
	return ((uint64_t)to_le32(s->blocks_count_hi) << 32) |
	       to_le32(s->blocks_count_lo);
}

/**
 * @brief   Blocks count set  in superblock.
 * @param   s superblock descriptor
 * @param   cnt count of blocks
 * */
static inline void ext4_sb_set_blocks_cnt(struct ext4_sblock *s, uint64_t cnt)
{
	s->blocks_count_lo = to_le32((cnt << 32) >> 32);
	s->blocks_count_hi = to_le32(cnt >> 32);
}

/**
 * @brief   Free blocks count get stored in superblock.
 * @param   s superblock descriptor
 * @return  free blocks
 **/
static inline uint64_t ext4_sb_get_free_blocks_cnt(struct ext4_sblock *s)
{
	return ((uint64_t)to_le32(s->free_blocks_count_hi) << 32) |
	       to_le32(s->free_blocks_count_lo);
}

/**
 * @brief   Free blocks count set.
 * @param   s superblock descriptor
 * @param   cnt new value of free blocks
 **/
static inline void ext4_sb_set_free_blocks_cnt(struct ext4_sblock *s,
					       uint64_t cnt)
{
	s->free_blocks_count_lo = to_le32((cnt << 32) >> 32);
	s->free_blocks_count_hi = to_le32(cnt >> 32);
}

/**
 * @brief   Block size get from superblock.
 * @param   s superblock descriptor
 * @return  block size in bytes
 **/
static inline uint32_t ext4_sb_get_block_size(struct ext4_sblock *s)
{
	return 1024 << to_le32(s->log_block_size);
}

/**
 * @brief   Block group descriptor size.
 * @param   s superblock descriptor
 * @return  block group descriptor size in bytes
 **/
static inline uint16_t ext4_sb_get_desc_size(struct ext4_sblock *s)
{
	uint16_t size = to_le16(s->desc_size);

	return size < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE
		   ? EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE
		   : size;
}

/*************************Flags and features*********************************/

/**
 * @brief   Support check of flag.
 * @param   s superblock descriptor
 * @param   v flag to check
 * @return  true if flag is supported
 **/
static inline bool ext4_sb_check_flag(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->flags) & v;
}

/**
 * @brief   Support check of feature compatible.
 * @param   s superblock descriptor
 * @param   v feature to check
 * @return  true if feature is supported
 **/
static inline bool ext4_sb_feature_com(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->features_compatible) & v;
}

/**
 * @brief   Support check of feature incompatible.
 * @param   s superblock descriptor
 * @param   v feature to check
 * @return  true if feature is supported
 **/
static inline bool ext4_sb_feature_incom(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->features_incompatible) & v;
}

/**
 * @brief   Support check of read only flag.
 * @param   s superblock descriptor
 * @param   v flag to check
 * @return  true if flag is supported
 **/
static inline bool ext4_sb_feature_ro_com(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->features_read_only) & v;
}

/**
 * @brief   Block group to flex group.
 * @param   s superblock descriptor
 * @param   block_group block group
 * @return  flex group id
 **/
static inline uint32_t ext4_sb_bg_to_flex(struct ext4_sblock *s,
					  uint32_t block_group)
{
	return block_group >> to_le32(s->log_groups_per_flex);
}

/**
 * @brief   Flex block group size.
 * @param   s superblock descriptor
 * @return  flex bg size
 **/
static inline uint32_t ext4_sb_flex_bg_size(struct ext4_sblock *s)
{
	return 1 << to_le32(s->log_groups_per_flex);
}

/**
 * @brief   Return first meta block group id.
 * @param   s superblock descriptor
 * @return  first meta_bg id
 **/
static inline uint32_t ext4_sb_first_meta_bg(struct ext4_sblock *s)
{
	return to_le32(s->first_meta_bg);
}

uint32_t ext4_block_group_cnt(struct ext4_sblock* s);
uint32_t ext4_blocks_in_group_cnt(struct ext4_sblock* s, uint32_t bgid);
uint32_t ext4_inodes_in_group_cnt(struct ext4_sblock* s, uint32_t bgid);
bool ext4_sb_check(struct ext4_sblock* s);
bool ext4_sb_sparse(uint32_t group);
bool ext4_sb_is_super_in_bg(struct ext4_sblock* s, uint32_t group);
#endif