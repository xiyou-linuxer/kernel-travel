#ifndef EXT4_EXTENT_H_
#define EXT4_EXTENT_H_

#include <xkernel/types.h>
#include <fs/ext4_inode.h>

struct ext4_extent_path {
	ext4_fsblk_t p_block;					// 物理块号，指向扩展树节点
	struct ext4_block block;				// 与扩展路径关联的块结构
	int32_t depth;							// 当前深度
	int32_t maxdepth;						// 最大深度
	struct ext4_extent_header *header;		// 扩展头指针
	struct ext4_extent_index *index;		// 扩展索引指针
	struct ext4_extent *extent;				// 扩展指针
	bool flag;								//标记该结构是否已经被分配
};
struct ext4_extent {
	uint32_t first_block; /* extent 覆盖的第一个逻辑块 */
	uint16_t block_count; /* extent 覆盖的块数 */
	uint16_t start_hi;    /* 物理块号的高 16 位 */
	uint32_t start_lo;    /* 物理块号的低 32 位 */
};

struct ext4_extent_header {
	uint16_t magic;					// 魔术数，用于识别扩展头
	uint16_t entries_count;			// 有效条目数
	uint16_t max_entries_count;		// 条目存储容量
	uint16_t depth;					// 树的深度，指示是否有实际的底层块
	uint32_t generation;			// 树的生成代数
};

struct ext4_extent_index {
	uint32_t first_block; /* 索引覆盖从此逻辑块开始的逻辑块 */
	/**
	 * 指向下一级物理块的指针。
	 * 叶子节点或下一级索引可能位于此处*/
	uint32_t leaf_lo;//物理块地址。
	uint16_t leaf_hi;//物理块的高16位。
	uint16_t padding; // 填充以对齐结构体
};

struct ext4_extent_tail
{
	uint32_t et_checksum; /* crc32c(uuid+inum+extent_block) */
};


#define EXT4_EXTENT_MAGIC 0xF30A

#define EXT4_EXTENT_FIRST(header)                                              \
    ((struct ext4_extent *)(((char *)(header)) +                           \
                sizeof(struct ext4_extent_header)))

#define EXT4_EXTENT_FIRST_INDEX(header)                                        \
    ((struct ext4_extent_index *)(((char *)(header)) +                     \
                      sizeof(struct ext4_extent_header)))
#define EXT_INIT_MAX_LEN (1L << 15)
#define EXT_UNWRITTEN_MAX_LEN (EXT_INIT_MAX_LEN - 1)

#define EXT_EXTENT_SIZE sizeof(struct ext4_extent)
#define EXT_INDEX_SIZE sizeof(struct ext4_extent_idx)

#define EXT_FIRST_EXTENT(__hdr__)                                              \
    ((struct ext4_extent *)(((char *)(__hdr__)) +                          \
                sizeof(struct ext4_extent_header)))
#define EXT_FIRST_INDEX(__hdr__)                                               \
    ((struct ext4_extent_index *)(((char *)(__hdr__)) +                    \
                    sizeof(struct ext4_extent_header)))
#define EXT_HAS_FREE_INDEX(__path__)                                           \
    (to_le16((__path__)->header->entries_count) <                                \
                    to_le16((__path__)->header->max_entries_count))
#define EXT_LAST_EXTENT(__hdr__)                                               \
    (EXT_FIRST_EXTENT((__hdr__)) + to_le16((__hdr__)->entries_count) - 1)
#define EXT_LAST_INDEX(__hdr__)                                                \
    (EXT_FIRST_INDEX((__hdr__)) + to_le16((__hdr__)->entries_count) - 1)
#define EXT_MAX_EXTENT(__hdr__)                                                \
    (EXT_FIRST_EXTENT((__hdr__)) + to_le16((__hdr__)->max_entries_count) - 1)
#define EXT_MAX_INDEX(__hdr__)                                                 \
    (EXT_FIRST_INDEX((__hdr__)) + to_le16((__hdr__)->max_entries_count) - 1)

#define EXT4_EXTENT_TAIL_OFFSET(hdr)                                           \
    (sizeof(struct ext4_extent_header) +                                   \
     (sizeof(struct ext4_extent) * to_le16((hdr)->max_entries_count)))

#define EXT4_EXT_MARK_UNWRIT1 0x02 /* 标记第一半为未写入状态 */
#define EXT4_EXT_MARK_UNWRIT2 0x04 /* 标记第二半为未写入状态 */
#define EXT4_EXT_DATA_VALID1 0x08  /* 第一半包含有效数据 */
#define EXT4_EXT_DATA_VALID2 0x10  /* 第二半包含有效数据 */
#define EXT4_EXT_NO_COMBINE 0x20   /* 不要合并两个扩展 */
#define EXT4_EXT_UNWRITTEN_MASK (1L << 15)

#define EXT4_EXT_MAX_LEN_WRITTEN (1L << 15)
#define EXT4_EXT_MAX_LEN_UNWRITTEN \
    (EXT4_EXT_MAX_LEN_WRITTEN - 1)

#define EXT4_EXT_GET_LEN(ex) to_le16((ex)->block_count)
#define EXT4_EXT_GET_LEN_UNWRITTEN(ex) \
    (EXT4_EXT_GET_LEN(ex) & ~(EXT4_EXT_UNWRITTEN_MASK))
#define EXT4_EXT_SET_LEN(ex, count) \
    ((ex)->block_count = to_le16(count))

#define EXT4_EXT_IS_UNWRITTEN(ex) \
    (EXT4_EXT_GET_LEN(ex) > EXT4_EXT_MAX_LEN_WRITTEN)
#define EXT4_EXT_SET_UNWRITTEN(ex) \
    ((ex)->block_count |= to_le16(EXT4_EXT_UNWRITTEN_MASK))
#define EXT4_EXT_SET_WRITTEN(ex) \
    ((ex)->block_count &= ~(to_le16(EXT4_EXT_UNWRITTEN_MASK)))

int ext4_extent_get_blocks(struct ext4_inode_ref *inode_ref, ext4_lblk_t iblock, uint32_t max_blocks, ext4_fsblk_t *result, bool create, uint32_t *blocks_count);
void ext4_extent_tree_init(struct ext4_inode_ref* inode_ref);
#endif