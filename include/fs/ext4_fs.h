#ifndef _EXT4_FS_H_
#define _EXT4_FS_H_

#include <xkernel/types.h>

int ext4_fs_get_inode_dblk_idx(struct ext4_inode_ref *inode_ref,uint64_t iblock, uint64_t *fblock,bool support_unwritten);
#endif