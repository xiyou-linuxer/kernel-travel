#ifndef EXT4_EXTENT_H_
#define EXT4_EXTENT_H_

#include <xkernel/types.h>
#include <fs/ext4_inode.h>

int ext4_extent_get_blocks(struct ext4_inode_ref *inode_ref, ext4_lblk_t iblock, uint32_t max_blocks, ext4_fsblk_t *result, bool create, uint32_t *blocks_count);

#endif