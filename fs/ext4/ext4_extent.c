#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <fs/fs.h>
#include <fs/fd.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/mount.h>
#include <fs/buf.h>
#include <fs/ext4.h>
#include <fs/ext4_sb.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_fs.h>
#include <xkernel/string.h>
#include <sync.h>
#include <debug.h>

struct ext4_extent_path {
	ext4_fsblk_t p_block;
	struct ext4_block block;
	int32_t depth;
	int32_t maxdepth;
	struct ext4_extent_header *header;
	struct ext4_extent_index *index;
	struct ext4_extent *extent;
};

struct ext4_extent {
	uint32_t first_block; /* extent 覆盖的第一个逻辑块 */
	uint16_t block_count; /* extent 覆盖的块数 */
	uint16_t start_hi;    /* 物理块号的高 16 位 */
	uint32_t start_lo;    /* 物理块号的低 32 位 */
};

struct ext4_extent_header {
    uint16_t magic;
    uint16_t entries_count;     /* Number of valid entries */
    uint16_t max_entries_count; /* Capacity of store in entries */
    uint16_t depth;             /* Has tree real underlying blocks? */
    uint32_t generation;    /* generation of the tree */
};

struct ext4_extent_index {
    uint32_t first_block; /* Index covers logical blocks from 'block' */

    /**
     * Pointer to the physical block of the next
     * level. leaf or next index could be there
     * high 16 bits of physical block
     */
    uint32_t leaf_lo;
    uint16_t leaf_hi;
    uint16_t padding;
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

/*
 * EXT_INIT_MAX_LEN is the maximum number of blocks we can have in an
 * initialized extent. This is 2^15 and not (2^16 - 1), since we use the
 * MSB of ee_len field in the extent datastructure to signify if this
 * particular extent is an initialized extent or an uninitialized (i.e.
 * preallocated).
 * EXT_UNINIT_MAX_LEN is the maximum number of blocks we can have in an
 * uninitialized extent.
 * If ee_len is <= 0x8000, it is an initialized extent. Otherwise, it is an
 * uninitialized one. In other words, if MSB of ee_len is set, it is an
 * uninitialized extent with only one special scenario when ee_len = 0x8000.
 * In this case we can not have an uninitialized extent of zero length and
 * thus we make it as a special case of initialized extent with 0x8000 length.
 * This way we get better extent-to-group alignment for initialized extents.
 * Hence, the maximum number of blocks we can have in an *initialized*
 * extent is 2^15 (32768) and in an *uninitialized* extent is 2^15-1 (32767).
 */
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


/**@brief Get logical number of the block covered by extent.
 * @param extent Extent to load number from
 * @return Logical number of the first block covered by extent */
static inline uint32_t ext4_extent_get_first_block(struct ext4_extent *extent)
{
    return to_le32(extent->first_block);
}

/**@brief Set logical number of the first block covered by extent.
 * @param extent Extent to set number to
 * @param iblock Logical number of the first block covered by extent */
static inline void ext4_extent_set_first_block(struct ext4_extent *extent,
        uint32_t iblock)
{
    extent->first_block = to_le32(iblock);
}

/**@brief Get number of blocks covered by extent.
 * @param extent Extent to load count from
 * @return Number of blocks covered by extent */
static inline uint16_t ext4_extent_get_block_count(struct ext4_extent *extent)
{
    if (EXT4_EXT_IS_UNWRITTEN(extent))
        return EXT4_EXT_GET_LEN_UNWRITTEN(extent);
    else
        return EXT4_EXT_GET_LEN(extent);
}
/**@brief Set number of blocks covered by extent.
 * @param extent Extent to load count from
 * @param count  Number of blocks covered by extent
 * @param unwritten Whether the extent is unwritten or not */
static inline void ext4_extent_set_block_count(struct ext4_extent *extent,
                           uint16_t count, bool unwritten)
{
    EXT4_EXT_SET_LEN(extent, count);
    if (unwritten)
        EXT4_EXT_SET_UNWRITTEN(extent);
}

/**@brief Get physical number of the first block covered by extent.
 * @param extent Extent to load number
 * @return Physical number of the first block covered by extent */
static inline uint64_t ext4_extent_get_start(struct ext4_extent *extent)
{
    return ((uint64_t)to_le16(extent->start_hi)) << 32 |
           ((uint64_t)to_le32(extent->start_lo));
}


/**@brief Set physical number of the first block covered by extent.
 * @param extent Extent to load number
 * @param fblock Physical number of the first block covered by extent */
static inline void ext4_extent_set_start(struct ext4_extent *extent, uint64_t fblock)
{
    extent->start_lo = to_le32((fblock << 32) >> 32);
    extent->start_hi = to_le16((uint16_t)(fblock >> 32));
}


/**@brief Get logical number of the block covered by extent index.
 * @param index Extent index to load number from
 * @return Logical number of the first block covered by extent index */
static inline uint32_t
ext4_extent_index_get_first_block(struct ext4_extent_index *index)
{
    return to_le32(index->first_block);
}

/**@brief Set logical number of the block covered by extent index.
 * @param index  Extent index to set number to
 * @param iblock Logical number of the first block covered by extent index */
static inline void
ext4_extent_index_set_first_block(struct ext4_extent_index *index,
                  uint32_t iblock)
{
    index->first_block = to_le32(iblock);
}

/**@brief Get physical number of block where the child node is located.
 * @param index Extent index to load number from
 * @return Physical number of the block with child node */
static inline uint64_t
ext4_extent_index_get_leaf(struct ext4_extent_index *index)
{
    return ((uint64_t)to_le16(index->leaf_hi)) << 32 |
           ((uint64_t)to_le32(index->leaf_lo));
}

/**@brief Set physical number of block where the child node is located.
 * @param index  Extent index to set number to
 * @param fblock Ohysical number of the block with child node */
static inline void ext4_extent_index_set_leaf(struct ext4_extent_index *index,
                          uint64_t fblock)
{
    index->leaf_lo = to_le32((fblock << 32) >> 32);
    index->leaf_hi = to_le16((uint16_t)(fblock >> 32));
}

/**@brief Get magic value from extent header.
 * @param header Extent header to load value from
 * @return Magic value of extent header */
static inline uint16_t
ext4_extent_header_get_magic(struct ext4_extent_header *header)
{
    return to_le16(header->magic);
}

/**@brief Set magic value to extent header.
 * @param header Extent header to set value to
 * @param magic  Magic value of extent header */
static inline void ext4_extent_header_set_magic(struct ext4_extent_header *header,
                        uint16_t magic)
{
    header->magic = to_le16(magic);
}

/**@brief Get number of entries from extent header
 * @param header Extent header to get value from
 * @return Number of entries covered by extent header */
static inline uint16_t
ext4_extent_header_get_entries_count(struct ext4_extent_header *header)
{
    return to_le16(header->entries_count);
}

/**@brief Set number of entries to extent header
 * @param header Extent header to set value to
 * @param count  Number of entries covered by extent header */
static inline void
ext4_extent_header_set_entries_count(struct ext4_extent_header *header,
                     uint16_t count)
{
    header->entries_count = to_le16(count);
}

/**@brief Get maximum number of entries from extent header
 * @param header Extent header to get value from
 * @return Maximum number of entries covered by extent header */
static inline uint16_t
ext4_extent_header_get_max_entries_count(struct ext4_extent_header *header)
{
    return to_le16(header->max_entries_count);
}

/**@brief Set maximum number of entries to extent header
 * @param header    Extent header to set value to
 * @param max_count Maximum number of entries covered by extent header */
static inline void
ext4_extent_header_set_max_entries_count(struct ext4_extent_header *header,
                          uint16_t max_count)
{
    header->max_entries_count = to_le16(max_count);
}

/**@brief Get depth of extent subtree.
 * @param header Extent header to get value from
 * @return Depth of extent subtree */
static inline uint16_t
ext4_extent_header_get_depth(struct ext4_extent_header *header)
{
    return to_le16(header->depth);
}

/**@brief Set depth of extent subtree.
 * @param header Extent header to set value to
 * @param depth  Depth of extent subtree */
static inline void
ext4_extent_header_set_depth(struct ext4_extent_header *header, uint16_t depth)
{
    header->depth = to_le16(depth);
}

/**@brief Get generation from extent header
 * @param header Extent header to get value from
 * @return Generation */
static inline uint32_t
ext4_extent_header_get_generation(struct ext4_extent_header *header)
{
    return to_le32(header->generation);
}

/**@brief Set generation to extent header
 * @param header     Extent header to set value to
 * @param generation Generation */
static inline void
ext4_extent_header_set_generation(struct ext4_extent_header *header,
                       uint32_t generation)
{
    header->generation = to_le32(generation);
}


void ext4_extent_tree_init(struct ext4_inode_ref *inode_ref)
{
    /* Initialize extent root header */
    struct ext4_extent_header *header =
            ext4_inode_get_extent_header(inode_ref->inode);
    ext4_extent_header_set_depth(header, 0);
    ext4_extent_header_set_entries_count(header, 0);
    ext4_extent_header_set_generation(header, 0);
    ext4_extent_header_set_magic(header, EXT4_EXTENT_MAGIC);

    uint16_t max_entries = (EXT4_INODE_BLOCKS * sizeof(uint32_t) -
            sizeof(struct ext4_extent_header)) /
                    sizeof(struct ext4_extent);

    ext4_extent_header_set_max_entries_count(header, max_entries);
    inode_ref->dirty  = true;
}


static struct ext4_extent_tail *
find_ext4_extent_tail(struct ext4_extent_header *eh)
{
	return (struct ext4_extent_tail *)(((char *)eh) +
					   EXT4_EXTENT_TAIL_OFFSET(eh));
}

static struct ext4_extent_header *ext_inode_hdr(struct ext4_inode *inode)
{
	return (struct ext4_extent_header *)inode->blocks;
}

static struct ext4_extent_header *ext_block_hdr(struct ext4_block *block)
{
	return (struct ext4_extent_header *)block->buf->data;
}

static uint16_t ext_depth(struct ext4_inode *inode)
{
	return to_le16(ext_inode_hdr(inode)->depth);
}

static uint16_t ext4_ext_get_actual_len(struct ext4_extent *ext)
{
	return (to_le16(ext->block_count) <= EXT_INIT_MAX_LEN
		    ? to_le16(ext->block_count)
		    : (to_le16(ext->block_count) - EXT_INIT_MAX_LEN));
}

static void ext4_ext_mark_initialized(struct ext4_extent *ext)
{
	ext->block_count = to_le16(ext4_ext_get_actual_len(ext));
}

static void ext4_ext_mark_unwritten(struct ext4_extent *ext)
{
	ext->block_count |= to_le16(EXT_INIT_MAX_LEN);
}

static int ext4_ext_is_unwritten(struct ext4_extent *ext)
{
	/* Extent with ee_len of 0x8000 is treated as an initialized extent */
	return (to_le16(ext->block_count) > EXT_INIT_MAX_LEN);
}
#define ext4_ext_block_csum(...) 0
static ext4_fsblk_t ext4_ext_pblock(struct ext4_extent *ex)
{
	ext4_fsblk_t block;

	block = to_le32(ex->start_lo);
	block |= ((ext4_fsblk_t)to_le16(ex->start_hi) << 31) << 1;
	return block;
}

ext4_extent_block_csum_set(struct ext4_inode_ref *inode_ref __unused,
			   struct ext4_extent_header *eh)
{
	struct ext4_extent_tail *tail;

	tail = find_ext4_extent_tail(eh);
	tail->et_checksum = to_le32(ext4_ext_block_csum(inode_ref, eh));
}


static void ext4_ext_drop_refs(struct ext4_inode_ref *inode_ref,
			       struct ext4_extent_path *path, bool keep_other)
{
	int32_t depth, i;

	if (!path)
		return;
	if (keep_other)
		depth = 0;
	else
		depth = path->depth;

	for (i = 0; i <= depth; i++, path++) {
		if (path->block.lb_id) {
			if (ext4_bcache_test_flag(path->block.buf, BC_DIRTY))
				ext4_extent_block_csum_set(inode_ref,
							   path->header);

			ext4_block_set(inode_ref->fs->bdev, &path->block);
		}
	}
}

static int ext4_find_extent(struct ext4_inode_ref *inode_ref, ext4_lblk_t block,
			    struct ext4_extent_path **orig_path, uint32_t flags)
{
	struct ext4_extent_header *eh;
	struct ext4_block bh = EXT4_BLOCK_ZERO();
	ext4_fsblk_t buf_block = 0;
	struct ext4_extent_path *path = *orig_path;
	int32_t depth, ppos = 0;
	int32_t i;
	int ret;

	eh = ext_inode_hdr(inode_ref->inode);
	depth = ext_depth(inode_ref->inode);

	if (path) {
		ext4_ext_drop_refs(inode_ref, path, 0);
		if (depth > path[0].maxdepth) {
			ext4_free(path);
			*orig_path = path = NULL;
		}
	}
	if (!path) {
		int32_t path_depth = depth + 1;
		/* account possible depth increase */
		path = ext4_calloc(1, sizeof(struct ext4_extent_path) *
				     (path_depth + 1));
		if (!path)
			return ENOMEM;
		path[0].maxdepth = path_depth;
	}
	path[0].header = eh;
	path[0].block = bh;

	i = depth;
	/* walk through the tree */
	while (i) {
		ext4_ext_binsearch_idx(path + ppos, block);
		path[ppos].p_block = ext4_idx_pblock(path[ppos].index);
		path[ppos].depth = i;
		path[ppos].extent = NULL;
		buf_block = path[ppos].p_block;

		i--;
		ppos++;
		if (!path[ppos].block.lb_id ||
		    path[ppos].block.lb_id != buf_block) {
			ret = read_extent_tree_block(inode_ref, buf_block, i,
						     &bh, flags);
			if (ret != EOK) {
				goto err;
			}
			if (ppos > depth) {
				ext4_block_set(inode_ref->fs->bdev, &bh);
				ret = EIO;
				goto err;
			}

			eh = ext_block_hdr(&bh);
			path[ppos].block = bh;
			path[ppos].header = eh;
		}
	}

	path[ppos].depth = i;
	path[ppos].extent = NULL;
	path[ppos].index = NULL;

	/* find extent */
	ext4_ext_binsearch(path + ppos, block);
	/* if not an empty leaf */
	if (path[ppos].extent)
		path[ppos].p_block = ext4_ext_pblock(path[ppos].extent);

	*orig_path = path;

	ret = EOK;
	return ret;

err:
	ext4_ext_drop_refs(inode_ref, path, 0);
	ext4_free(path);
	if (orig_path)
		*orig_path = NULL;
	return ret;
}


int ext4_extent_get_blocks(struct ext4_inode_ref *inode_ref, ext4_lblk_t iblock, uint32_t max_blocks, ext4_fsblk_t *result, bool create, uint32_t *blocks_count)
{
	struct ext4_extent_path *path = NULL;   // 用于存储找到的extent路径
	struct ext4_extent newex, *ex;          // 定义一个新的extent和一个指向现有extent的指针
	ext4_fsblk_t goal;                      // 定义目标块
	int EOK = 0;
	int err = EOK;                          // 初始化错误码为EOK
	int32_t depth;                          // 定义extent树的深度
	uint32_t allocated = 0;                 // 初始化已分配的块数为0
	ext4_lblk_t next;                       // 定义下一个逻辑块
	ext4_fsblk_t newblock;                  // 定义新的块号

	if (result)
		*result = 0;                       // 如果result非空，将其初始化为0

	if (blocks_count)
		*blocks_count = 0;                 // 如果blocks_count非空，将其初始化为0

	// 查找指定逻辑块对应的extent
	err = ext4_find_extent(inode_ref, iblock, &path, 0);
	if (err != EOK) {
		path = NULL;                      // 如果查找失败，将路径置为空
		goto out2;                        // 跳转到out2标签进行清理和返回
	}

	depth = ext_depth(inode_ref->inode);    // 获取extent树的深度

	/*
	 * 一致的叶节点不能为空
	 * 在树修改期间，这种情况是可能的
	 * 因此不能在ext4_ext_find_extent()中使用断言
	 */
	ex = path[depth].extent;                // 获取树中最深层次的extent
	if (ex) {
		ext4_lblk_t ee_block = to_le32(ex->first_block); // 获取extent的起始逻辑块号
		ext4_fsblk_t ee_start = ext4_ext_pblock(ex);      // 获取extent的起始物理块号
		uint16_t ee_len = ext4_ext_get_actual_len(ex);    // 获取extent的长度

		// 如果找到的extent覆盖了请求的块号，直接返回该块号
		if (IN_RANGE(iblock, ee_block, ee_len)) {
			// 计算extent中剩余的块数
			allocated = ee_len - (iblock - ee_block);

			if (!ext4_ext_is_unwritten(ex)) {              // 如果extent不是未写入的
				newblock = iblock - ee_block + ee_start;   // 计算对应的物理块号
				goto out;                                // 跳转到out标签进行返回
			}

			if (!create) {                                // 如果不允许创建新块
				newblock = 0;                             // 将新块号置为0
				goto out;                                // 跳转到out标签进行返回
			}

			uint32_t zero_range;
			zero_range = allocated;                       // 将需要置零的范围设置为已分配的块数
			if (zero_range > max_blocks)
				zero_range = max_blocks;                // 如果需要置零的范围大于最大块数，将其限制为最大块数

			newblock = iblock - ee_block + ee_start;      // 计算对应的物理块号
			err = ext4_ext_zero_unwritten_range(inode_ref, newblock,
							    zero_range); // 置零未写入的范围
			if (err != EOK)
				goto out2;                              // 如果出错，跳转到out2标签进行清理和返回

			err = ext4_ext_convert_to_initialized(
			    inode_ref, &path, iblock, zero_range);   // 将未写入的extent转换为已初始化的extent
			if (err != EOK)
				goto out2;                              // 如果出错，跳转到out2标签进行清理和返回

			goto out;                                    // 跳转到out标签进行返回
		}
	}

	/*
	 * 如果请求的块尚未分配且不允许创建新块，则返回错误
	 */
	if (!create) {
		goto out2;                                        // 跳转到out2标签进行清理和返回
	}

	// 查找下一个已分配的块，以便知道可以分配多少块而不重叠下一个extent
	next = ext4_ext_next_allocated_block(path);
	allocated = next - iblock;                            // 计算可以分配的块数
	if (allocated > max_blocks)
		allocated = max_blocks;                         // 如果可以分配的块数大于最大块数，将其限制为最大块数

	/* 分配新块 */
	goal = ext4_ext_find_goal(inode_ref, path, iblock);    // 查找分配新块的目标位置
	newblock = ext4_new_meta_blocks(inode_ref, goal, 0, &allocated, &err); // 分配新块
	if (!newblock)
		goto out2;                                        // 如果分配失败，跳转到out2标签进行清理和返回

	/* 尝试将新extent插入到找到的叶节点并返回 */
	newex.first_block = to_le32(iblock);                   // 设置新extent的起始逻辑块号
	ext4_ext_store_pblock(&newex, newblock);               // 存储新extent的起始物理块号
	newex.block_count = to_le16(allocated);                // 设置新extent的块数
	err = ext4_ext_insert_extent(inode_ref, &path, &newex, 0); // 插入新extent
	if (err != EOK) {
		/* 如果插入失败，释放刚刚分配的数据块 */
		ext4_ext_free_blocks(inode_ref, ext4_ext_pblock(&newex),
				     to_le16(newex.block_count), 0);
		goto out2;                                        // 跳转到out2标签进行清理和返回
	}

	/* 前面的步骤可能已经使用了我们分配的块 */
	newblock = ext4_ext_pblock(&newex);                    // 获取新extent的起始物理块号

out:
	if (allocated > max_blocks)
		allocated = max_blocks;                           // 如果已分配的块数大于最大块数，将其限制为最大块数

	if (result)
		*result = newblock;                               // 如果result非空，将其设置为新块号

	if (blocks_count)
		*blocks_count = allocated;                        // 如果blocks_count非空，将其设置为已分配的块数

out2:
	if (path) {
		ext4_ext_drop_refs(inode_ref, path, 0);           // 释放路径的引用
		ext4_free(path);                                  // 释放路径的内存
	}

	return err;                                             // 返回错误码
}
