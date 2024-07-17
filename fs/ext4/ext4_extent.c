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
#include <fs/ext4_block_group.h>
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
    uint16_t entries_count;     /* 有效条目数 */
    uint16_t max_entries_count; /* 条目存储容量 */
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

static ext4_fsblk_t ext4_ext_find_goal(struct ext4_inode_ref *inode_ref,struct ext4_extent_path *path,ext4_lblk_t block);

/**
 * @brief 获取范围所覆盖的块的逻辑编号。
 * @param 范围加载数字的范围
 * @return 被extent覆盖的第一个块的逻辑编号 
 **/
static inline uint32_t ext4_extent_get_first_block(struct ext4_extent *extent)
{
    return to_le32(extent->first_block);
}

/**
 * @brief 设置范围覆盖的第一个块的逻辑编号。
 * @param 范围 设置数字的范围
 * @param iblock 被extent覆盖的第一个块的逻辑号 
 **/
static inline void ext4_extent_set_first_block(struct ext4_extent *extent,
        uint32_t iblock)
{
    extent->first_block = to_le32(iblock);
}

/**
 * @brief 获取范围覆盖的块数。
 * @param范围加载计数的范围
 * @return 范围覆盖的块数
 **/
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

static void ext4_ext_free_blocks(struct ext4_inode_ref *inode_ref,
				 ext4_fsblk_t block, uint32_t count,
				 uint32_t flags  __attribute__ ((__unused__)))
{
	ext4_balloc_free_blocks(inode_ref, block, count);
}

static int ext4_ext_dirty(struct ext4_inode_ref *inode_ref,
			  struct ext4_extent_path *path)
{
	if (path->block.lb_id)
		path->block.buf->dirty = 1;
	else
		inode_ref->dirty = true;

	return 0;
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
	return (struct ext4_extent_header *)block->buf->data->data;
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

#define ext4_ext_block_csum(...) 0
static ext4_fsblk_t ext4_ext_pblock(struct ext4_extent *ex)
{
	ext4_fsblk_t block;

	block = to_le32(ex->start_lo);
	block |= ((ext4_fsblk_t)to_le16(ex->start_hi) << 31) << 1;
	return block;
}

static int ext4_allocate_single_block(struct ext4_inode_ref *inode_ref,
				      ext4_fsblk_t goal, ext4_fsblk_t *blockp)
{
	return ext4_balloc_alloc_block(inode_ref, goal, blockp);
}

void ext4_extent_block_csum_set(struct ext4_inode_ref *inode_ref __attribute__ ((__unused__)),
			   struct ext4_extent_header *eh)
{
	struct ext4_extent_tail *tail;

	tail = find_ext4_extent_tail(eh);
	tail->et_checksum = to_le32(ext4_ext_block_csum(inode_ref, eh));
}

static void ext4_ext_store_pblock(struct ext4_extent *ex, ext4_fsblk_t pb)
{
	ex->start_lo = to_le32((uint32_t)(pb & 0xffffffff));
	ex->start_hi = to_le16((uint16_t)((pb >> 32)) & 0xffff);
}

ext4_fsblk_t ext4_fs_inode_to_goal_block(struct ext4_inode_ref *inode_ref)
{
	uint32_t grp_inodes = ext4_get32(&inode_ref->fs->superBlock.ext4_sblock, inodes_per_group);
	return (inode_ref->index - 1) / grp_inodes;
}

static void ext4_idx_store_pblock(struct ext4_extent_index *ix, ext4_fsblk_t pb)
{
	ix->leaf_lo = to_le32((uint32_t)(pb & 0xffffffff));
	ix->leaf_hi = to_le16((uint16_t)((pb >> 32)) & 0xffff);
}

static ext4_fsblk_t ext4_idx_pblock(struct ext4_extent_index *ix)
{
	ext4_fsblk_t block;

	block = to_le32(ix->leaf_lo);
	block |= ((ext4_fsblk_t)to_le16(ix->leaf_hi) << 31) << 1;
	return block;
}

static int ext4_ext_is_unwritten(struct ext4_extent *ext)
{
	/* Extent with ee_len of 0x8000 is treated as an initialized extent */
	return (to_le16(ext->block_count) > EXT_INIT_MAX_LEN);
}

static uint16_t ext4_ext_space_root_idx(struct ext4_inode_ref *inode_ref)
{
	uint16_t size;

	size = sizeof(inode_ref->inode->blocks);
	size -= sizeof(struct ext4_extent_header);
	size /= sizeof(struct ext4_extent_index);
#ifdef AGGRESSIVE_TEST
	if (size > 4)
		size = 4;
#endif
	return size;
}

static void ext4_ext_binsearch_idx(struct ext4_extent_path *path,
				   ext4_lblk_t block)
{
	struct ext4_extent_header *eh = path->header;
	struct ext4_extent_index *r, *l, *m;

	l = EXT_FIRST_INDEX(eh) + 1;
	r = EXT_LAST_INDEX(eh);
	while (l <= r) {
		m = l + (r - l) / 2;
		if (block < to_le32(m->first_block))
			r = m - 1;
		else
			l = m + 1;
	}

	path->index = l - 1;
}

static void ext4_ext_binsearch(struct ext4_extent_path *path, ext4_lblk_t block)
{
	struct ext4_extent_header *eh = path->header;
	struct ext4_extent *r, *l, *m;

	if (eh->entries_count == 0) {
		return;
	}

	l = EXT_FIRST_EXTENT(eh) + 1;
	r = EXT_LAST_EXTENT(eh);

	while (l <= r) {
		m = l + (r - l) / 2;
		if (block < to_le32(m->first_block))
			r = m - 1;
		else
			l = m + 1;
	}

	path->extent = l - 1;
}

static uint16_t ext4_ext_space_block_idx(struct ext4_inode_ref *inode_ref)
{
	uint16_t size;
	uint32_t block_size = ext4_sb_get_block_size(&inode_ref->fs->superBlock.ext4_sblock);

	size = (block_size - sizeof(struct ext4_extent_header)) /
	       sizeof(struct ext4_extent_index);
#ifdef AGGRESSIVE_TEST
	if (size > 5)
		size = 5;
#endif
	return size;
}


ext4_fsblk_t ext4_new_meta_blocks(struct ext4_inode_ref *inode_ref,
					 ext4_fsblk_t goal,
					 uint32_t flags __attribute__ ((__unused__)),
					 uint32_t *count, int *errp)
{
	ext4_fsblk_t block = 0;

	*errp = ext4_allocate_single_block(inode_ref, goal, &block);
	if (count)
		*count = 1;
	return block;
}



static int ext4_ext_check(struct ext4_inode_ref *inode_ref,
			  struct ext4_extent_header *eh, uint16_t depth,
			  ext4_fsblk_t pblk __attribute__ ((__unused__)))
{
	struct ext4_extent_tail *tail;
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
	const char *error_msg;
	(void)error_msg;

	if (to_le16(eh->magic) != EXT4_EXTENT_MAGIC) {
		error_msg = "invalid magic";
		goto corrupted;
	}
	if (to_le16(eh->depth) != depth) {
		error_msg = "unexpected eh_depth";
		goto corrupted;
	}
	if (eh->max_entries_count == 0) {
		error_msg = "invalid eh_max";
		goto corrupted;
	}
	if (to_le16(eh->entries_count) > to_le16(eh->max_entries_count)) {
		error_msg = "invalid eh_entries";
		goto corrupted;
	}

	tail = find_ext4_extent_tail(eh);
	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM)) {
		if (tail->et_checksum !=
		    to_le32(ext4_ext_block_csum(inode_ref, eh))) {
			printk("Extent block checksum failed.Blocknr: %d \n",pblk);
		}
	}

	return 0;

corrupted:
	printk("Bad extents B+ tree block: %s. Blocknr: %d\n",error_msg, pblk);
	return -1;
}

int read_extent_tree_block(struct ext4_inode_ref *inode_ref,
				  ext4_fsblk_t pblk, int32_t depth,
				  struct ext4_block *bh,
				  uint32_t flags __attribute__ ((__unused__)))
{
	int err;
	int EOK = 0;
	bh->buf = bufRead(1, EXT4_LBA2PBA(pblk),1);
	err = ext4_ext_check(inode_ref, ext_block_hdr(bh), depth, pblk);
	if (err != EOK)
		goto errout;

	return EOK;
errout:
	if (bh->lb_id)
		bufRelease(bh->buf);

	return err;
}

static uint16_t ext4_ext_space_block(struct ext4_inode_ref *inode_ref)
{
	uint16_t size;
	uint32_t block_size = ext4_sb_get_block_size(&inode_ref->fs->superBlock.ext4_sblock);

	size = (block_size - sizeof(struct ext4_extent_header)) /
	       sizeof(struct ext4_extent);
#ifdef AGGRESSIVE_TEST
	if (size > 6)
		size = 6;
#endif
	return size;
}

static inline bool ext4_ext_can_prepend(struct ext4_extent *ex1,
					struct ext4_extent *ex2)
{
	if (ext4_ext_pblock(ex2) + ext4_ext_get_actual_len(ex2) !=
	    ext4_ext_pblock(ex1))
		return 0;

#ifdef AGGRESSIVE_TEST
	if (ext4_ext_get_actual_len(ex1) + ext4_ext_get_actual_len(ex2) > 4)
		return 0;
#else
	if (ext4_ext_is_unwritten(ex1)) {
		if (ext4_ext_get_actual_len(ex1) +
			ext4_ext_get_actual_len(ex2) >
		    EXT_UNWRITTEN_MAX_LEN)
			return 0;
	} else if (ext4_ext_get_actual_len(ex1) + ext4_ext_get_actual_len(ex2) >
		   EXT_INIT_MAX_LEN)
		return 0;
#endif

	if (to_le32(ex2->first_block) + ext4_ext_get_actual_len(ex2) !=
	    to_le32(ex1->first_block))
		return 0;

	return 1;
}

static uint16_t ext4_ext_space_root(struct ext4_inode_ref *inode_ref)
{
	uint16_t size;

	size = sizeof(inode_ref->inode->blocks);
	size -= sizeof(struct ext4_extent_header);
	size /= sizeof(struct ext4_extent);
#ifdef AGGRESSIVE_TEST
	if (size > 3)
		size = 3;
#endif
	return size;
}

static uint16_t ext4_ext_max_entries(struct ext4_inode_ref *inode_ref,
				     uint32_t depth)
{
	uint16_t max;

	if (depth == ext_depth(inode_ref->inode)) {
		if (depth == 0)
			max = ext4_ext_space_root(inode_ref);
		else
			max = ext4_ext_space_root_idx(inode_ref);
	} else {
		if (depth == 0)
			max = ext4_ext_space_block(inode_ref);
		else
			max = ext4_ext_space_block_idx(inode_ref);
	}

	return max;
}

static int ext4_ext_correct_indexes(struct ext4_inode_ref *inode_ref,
				    struct ext4_extent_path *path)
{
	int EOK = 0;
	struct ext4_extent_header *eh;
	int32_t depth = ext_depth(inode_ref->inode);
	struct ext4_extent *ex;
	uint32_t border;
	int32_t k;
	int err = EOK;

	eh = path[depth].header;
	ex = path[depth].extent;

	if (ex == NULL || eh == NULL)
		return -1;

	if (depth == 0) {
		/* there is no tree at all */
		return EOK;
	}

	if (ex != EXT_FIRST_EXTENT(eh)) {
		/* we correct tree if first leaf got modified only */
		return EOK;
	}

	k = depth - 1;
	border = path[depth].extent->first_block;
	path[k].index->first_block = border;
	err = ext4_ext_dirty(inode_ref, path + k);
	if (err != EOK)
		return err;

	while (k--) {
		/* change all left-side indexes */
		if (path[k + 1].index != EXT_FIRST_INDEX(path[k + 1].header))
			break;
		path[k].index->first_block = border;
		err = ext4_ext_dirty(inode_ref, path + k);
		if (err != EOK)
			break;
	}

	return err;
}

static inline bool ext4_ext_can_append(struct ext4_extent *ex1,
				       struct ext4_extent *ex2)
{
	if (ext4_ext_pblock(ex1) + ext4_ext_get_actual_len(ex1) !=
	    ext4_ext_pblock(ex2))
		return 0;

#ifdef AGGRESSIVE_TEST
	if (ext4_ext_get_actual_len(ex1) + ext4_ext_get_actual_len(ex2) > 4)
		return 0;
#else
	if (ext4_ext_is_unwritten(ex1)) {
		if (ext4_ext_get_actual_len(ex1) +
			ext4_ext_get_actual_len(ex2) >
		    EXT_UNWRITTEN_MAX_LEN)
			return 0;
	} else if (ext4_ext_get_actual_len(ex1) + ext4_ext_get_actual_len(ex2) >
		   EXT_INIT_MAX_LEN)
		return 0;
#endif

	if (to_le32(ex1->first_block) + ext4_ext_get_actual_len(ex1) !=
	    to_le32(ex2->first_block))
		return 0;

	return 1;
}

static int ext4_ext_insert_leaf(struct ext4_inode_ref *inode_ref,
				struct ext4_extent_path *path, int at,
				struct ext4_extent *newext, int flags,
				bool *need_split)
{
	struct ext4_extent_path *curp = path + at;
	struct ext4_extent *ex = curp->extent;
	int len, err, unwritten;
	struct ext4_extent_header *eh;

	*need_split = false;

	if (curp->extent &&
	    to_le32(newext->first_block) == to_le32(curp->extent->first_block))
		return -1;

	if (!(flags & EXT4_EXT_NO_COMBINE)) {
		if (curp->extent && ext4_ext_can_append(curp->extent, newext)) {
			unwritten = ext4_ext_is_unwritten(curp->extent);
			curp->extent->block_count = to_le16(ext4_ext_get_actual_len(curp->extent) + ext4_ext_get_actual_len(newext));
			if (unwritten)
				ext4_ext_mark_unwritten(curp->extent);

			err = ext4_ext_dirty(inode_ref, curp);
			goto out;
		}

		if (curp->extent &&
		    ext4_ext_can_prepend(curp->extent, newext)) {
			unwritten = ext4_ext_is_unwritten(curp->extent);
			curp->extent->first_block = newext->first_block;
			curp->extent->block_count = to_le16(ext4_ext_get_actual_len(curp->extent) + ext4_ext_get_actual_len(newext));
			if (unwritten)
				ext4_ext_mark_unwritten(curp->extent);

			err = ext4_ext_dirty(inode_ref, curp);
			goto out;
		}
	}

	if (to_le16(curp->header->entries_count) ==
	    to_le16(curp->header->max_entries_count)) {
		err = -1;
		*need_split = true;
		goto out;
	} else {
		eh = curp->header;
		if (curp->extent == NULL) {
			ex = EXT_FIRST_EXTENT(eh);
			curp->extent = ex;
		} else if (to_le32(newext->first_block) >
			   to_le32(curp->extent->first_block)) {
			/* insert after */
			ex = curp->extent + 1;
		} else {
			/* insert before */
			ex = curp->extent;
		}
	}

	len = EXT_LAST_EXTENT(eh) - ex + 1;
	ASSERT(len >= 0);
	if (len > 0)
		memcpy(ex + 1, ex, len * sizeof(struct ext4_extent));

	if (ex > EXT_MAX_EXTENT(eh)) {
		err = -1;
		goto out;
	}

	ex->first_block = newext->first_block;
	ex->block_count = newext->block_count;
	ext4_ext_store_pblock(ex, ext4_ext_pblock(newext));
	eh->entries_count = to_le16(to_le16(eh->entries_count) + 1);

	if (ex > EXT_LAST_EXTENT(eh)) {
		err = -1;
		goto out;
	}

	err = ext4_ext_correct_indexes(inode_ref, path);
	if (err != 0)
		goto out;
	
	err = ext4_ext_dirty(inode_ref, curp);

out:
	if (err == 0) {
		curp->extent = ex;
		curp->p_block = ext4_ext_pblock(ex);
	}

	return err;
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
			if (path->block.buf->dirty == 1)
				ext4_extent_block_csum_set(inode_ref,path->header);
			bufRelease(path->block.buf);
		}
	}
}

static int ext4_ext_insert_index(struct ext4_inode_ref *inode_ref,
				 struct ext4_extent_path *path, int at,
				 ext4_lblk_t insert_index,
				 ext4_fsblk_t insert_block, bool set_to_ix)
{
	int EIO = 5;
	struct ext4_extent_index *ix;
	struct ext4_extent_path *curp = path + at;
	int len, err;
	struct ext4_extent_header *eh;

	if (curp->index && insert_index == to_le32(curp->index->first_block))
		return EIO;

	if (to_le16(curp->header->entries_count) ==
	    to_le16(curp->header->max_entries_count))
		return EIO;

	eh = curp->header;
	if (curp->index == NULL) {
		ix = EXT_FIRST_INDEX(eh);
		curp->index = ix;
	} else if (insert_index > to_le32(curp->index->first_block)) {
		/* insert after */
		ix = curp->index + 1;
	} else {
		/* insert before */
		ix = curp->index;
	}

	if (ix > EXT_MAX_INDEX(eh))
		return EIO;

	len = EXT_LAST_INDEX(eh) - ix + 1;
	ASSERT(len >= 0);
	if (len > 0)
		memcpy(ix + 1, ix, len * sizeof(struct ext4_extent_index));

	ix->first_block = to_le32(insert_index);
	ext4_idx_store_pblock(ix, insert_block);
	eh->entries_count = to_le16(to_le16(eh->entries_count) + 1);

	if (ix > EXT_LAST_INDEX(eh)) {
		err = EIO;
		goto out;
	}

	err = ext4_ext_dirty(inode_ref, curp);

out:
	if (err == 0 && set_to_ix) {
		curp->index = ix;
		curp->p_block = ext4_idx_pblock(ix);
	}
	return err;
}


static int ext4_find_extent(struct ext4_inode_ref *inode_ref, ext4_lblk_t block,
			    struct ext4_extent_path **orig_path, uint32_t flags)
{
	int EOK = 0; // 表示操作成功的返回值
	struct ext4_extent_header *eh; // 扩展头部指针
	struct ext4_block bh = EXT4_BLOCK_ZERO(); // 初始化块
	ext4_fsblk_t buf_block = 0; // 缓冲块
	struct ext4_extent_path *path = *orig_path; // 扩展路径
	int32_t depth, ppos = 0; // 深度和路径位置指针
	int32_t i;
	int ret;

	// 获取inode的扩展头部和深度
	eh = ext_inode_hdr(inode_ref->inode);
	depth = ext_depth(inode_ref->inode);

	// 如果已有路径，释放其引用
	if (path) {
		ext4_ext_drop_refs(inode_ref, path, 0);
		// 如果当前深度大于路径的最大深度，释放路径
		if (depth > path[0].maxdepth) {
			ext4_free(path);
			*orig_path = path = NULL;
		}
	}

	// 如果没有路径，分配新的路径
	if (!path) {
		int32_t path_depth = depth + 1;
		// 考虑可能的深度增加
		path = ext4_calloc(1, sizeof(struct ext4_extent_path) *
				     (path_depth + 1));
		if (!path)
			return -1;
		path[0].maxdepth = path_depth;
	}

	// 设置路径的初始头部和块
	path[0].header = eh;
	path[0].block = bh;

	i = depth;
	// 遍历树
	while (i) {
		ext4_ext_binsearch_idx(path + ppos, block); // 二分查找索引
		path[ppos].p_block = ext4_idx_pblock(path[ppos].index); // 设置物理块
		path[ppos].depth = i; // 设置深度
		path[ppos].extent = NULL;
		buf_block = path[ppos].p_block; // 设置缓冲块

		i--;
		ppos++;
		// 如果块需要读取或不匹配，读取扩展树块
		if (!path[ppos].block.lb_id ||
		    path[ppos].block.lb_id != buf_block) {
			ret = read_extent_tree_block(inode_ref, buf_block, i,
						     &bh, flags);
			if (ret != EOK) {
				goto err;
			}
			// 超出最大深度，出错
			if (ppos > depth) {
				bufRelease(bh.buf);
				ret = -1;
				goto err;
			}

			// 更新头部和块
			eh = ext_block_hdr(&bh);
			path[ppos].block = bh;
			path[ppos].header = eh;
		}
	}

	path[ppos].depth = i;
	path[ppos].extent = NULL;
	path[ppos].index = NULL;

	// 查找扩展
	ext4_ext_binsearch(path + ppos, block);
	// 如果找到非空叶子节点，设置物理块
	if (path[ppos].extent)
		path[ppos].p_block = ext4_ext_pblock(path[ppos].extent);

	*orig_path = path; // 更新原始路径

	ret = EOK;
	return ret;

err:
	// 出错处理，释放引用和路径
	ext4_ext_drop_refs(inode_ref, path, 0);
	ext4_free(path);
	if (orig_path)
		*orig_path = NULL;
	return ret;
}

static int ext4_ext_grow_indepth(struct ext4_inode_ref *inode_ref,
				 uint32_t flags)
{
	int EOK = 0;
	struct ext4_extent_header *neh;
	struct ext4_block bh = EXT4_BLOCK_ZERO();
	ext4_fsblk_t newblock, goal = 0;
	int err = EOK;

	/* Try to prepend new index to old one */
	if (ext_depth(inode_ref->inode))
		goal = ext4_idx_pblock(
		    EXT_FIRST_INDEX(ext_inode_hdr(inode_ref->inode)));
	else
		goal = ext4_fs_inode_to_goal_block(inode_ref);

	newblock = ext4_new_meta_blocks(inode_ref, goal, flags, NULL, &err);
	if (newblock == 0)
		return err;

	/* # */
	bh.buf = bufRead(1,EXT4_LBA2PBA(newblock),1);

	/* move top-level index/leaf into new block */
	memcpy(bh.buf->data->data, inode_ref->inode->blocks,
		sizeof(inode_ref->inode->blocks));

	/* set size of new block */
	neh = ext_block_hdr(&bh);
	/* old root could have indexes or leaves
	 * so calculate e_max right way */
	if (ext_depth(inode_ref->inode))
		neh->max_entries_count =
		    to_le16(ext4_ext_space_block_idx(inode_ref));
	else
		neh->max_entries_count =
		    to_le16(ext4_ext_space_block(inode_ref));

	neh->magic = to_le16(EXT4_EXTENT_MAGIC);
	ext4_extent_block_csum_set(inode_ref, neh);

	/* Update top-level index: num,max,pointer */
	neh = ext_inode_hdr(inode_ref->inode);
	neh->entries_count = to_le16(1);
	ext4_idx_store_pblock(EXT_FIRST_INDEX(neh), newblock);
	if (neh->depth == 0) {
		/* Root extent block becomes index block */
		neh->max_entries_count =
		    to_le16(ext4_ext_space_root_idx(inode_ref));
		EXT_FIRST_INDEX(neh)
		    ->first_block = EXT_FIRST_EXTENT(neh)->first_block;
	}
	neh->depth = to_le16(to_le16(neh->depth) + 1);
	bh.buf->dirty = 1;
	inode_ref->dirty = true;
	bufRelease(bh.buf);
	return err;
}

static void ext4_ext_init_header(struct ext4_inode_ref *inode_ref,
				 struct ext4_extent_header *eh, int32_t depth)
{
	eh->entries_count = 0;
	eh->max_entries_count = to_le16(ext4_ext_max_entries(inode_ref, depth));
	eh->magic = to_le16(EXT4_EXTENT_MAGIC);
	eh->depth = depth;
}

static ext4_fsblk_t ext4_ext_new_meta_block(struct ext4_inode_ref *inode_ref,
					    struct ext4_extent_path *path,
					    struct ext4_extent *ex, int *err,
					    uint32_t flags)
{
	ext4_fsblk_t goal, newblock;

	goal = ext4_ext_find_goal(inode_ref, path, to_le32(ex->first_block));
	newblock = ext4_new_meta_blocks(inode_ref, goal, flags, NULL, err);
	return newblock;
}

static int ext4_ext_split_node(struct ext4_inode_ref *inode_ref,
			       struct ext4_extent_path *path, int at,
			       struct ext4_extent *newext,
			       struct ext4_extent_path *npath,
			       bool *ins_right_leaf)
{
	int EOK = 0;
	int i, npath_at, ret;
	ext4_lblk_t insert_index;
	ext4_fsblk_t newblock = 0;
	int depth = ext_depth(inode_ref->inode);
	npath_at = depth - at;

	ASSERT(at > 0);

	if (path[depth].extent != EXT_MAX_EXTENT(path[depth].header))
		insert_index = path[depth].extent[1].first_block;
	else
		insert_index = newext->first_block;

	for (i = depth; i >= at; i--, npath_at--) {
		struct ext4_block bh = EXT4_BLOCK_ZERO();

		/* FIXME: currently we split at the point after the current
		 * extent. */
		newblock =
		    ext4_ext_new_meta_block(inode_ref, path, newext, &ret, 0);
		if (ret != EOK)
			goto cleanup;

		/*  For write access.*/
		bh.buf = bufRead(1,EXT4_LBA2PBA(newblock),1);

		if (i == depth) {
			/* start copy from next extent */
			int m = EXT_MAX_EXTENT(path[i].header) - path[i].extent;
			struct ext4_extent_header *neh;
			struct ext4_extent *ex;
			neh = ext_block_hdr(&bh);
			ex = EXT_FIRST_EXTENT(neh);
			ext4_ext_init_header(inode_ref, neh, 0);
			if (m) {
				memcpy(ex, path[i].extent + 1, sizeof(struct ext4_extent) * m);
				neh->entries_count =
				    to_le16(to_le16(neh->entries_count) + m);
				path[i].header->entries_count = to_le16(
				    to_le16(path[i].header->entries_count) - m);
				ret = ext4_ext_dirty(inode_ref, path + i);
				if (ret != EOK)
					goto cleanup;

				npath[npath_at].p_block = ext4_ext_pblock(ex);
				npath[npath_at].extent = ex;
			} else {
				npath[npath_at].p_block = 0;
				npath[npath_at].extent = NULL;
			}

			npath[npath_at].depth = to_le16(neh->depth);
			npath[npath_at].maxdepth = 0;
			npath[npath_at].index = NULL;
			npath[npath_at].header = neh;
			npath[npath_at].block = bh;

			ext4_trans_set_block_dirty(bh.buf);
		} else {
			int m = EXT_MAX_INDEX(path[i].header) - path[i].index;
			struct ext4_extent_header *neh;
			struct ext4_extent_index *ix;
			neh = ext_block_hdr(&bh);
			ix = EXT_FIRST_INDEX(neh);
			ext4_ext_init_header(inode_ref, neh, depth - i);
			ix->first_block = to_le32(insert_index);
			ext4_idx_store_pblock(ix,
					      npath[npath_at + 1].block.lb_id);
			neh->entries_count = to_le16(1);
			if (m) {
				memcpy(ix + 1, path[i].index + 1,
					sizeof(struct ext4_extent) * m);
				neh->entries_count =
				    to_le16(to_le16(neh->entries_count) + m);
				path[i].header->entries_count = to_le16(
				    to_le16(path[i].header->entries_count) - m);
				ret = ext4_ext_dirty(inode_ref, path + i);
				if (ret != EOK)
					goto cleanup;
			}

			npath[npath_at].p_block = ext4_idx_pblock(ix);
			npath[npath_at].depth = to_le16(neh->depth);
			npath[npath_at].maxdepth = 0;
			npath[npath_at].extent = NULL;
			npath[npath_at].index = ix;
			npath[npath_at].header = neh;
			npath[npath_at].block = bh;

			ext4_trans_set_block_dirty(bh.buf);
		}
	}
	newblock = 0;

	/*
	 * If newext->first_block can be included into the
	 * right sub-tree.
	 */
	if (to_le32(newext->first_block) < insert_index)
		*ins_right_leaf = false;
	else
		*ins_right_leaf = true;

	ret = ext4_ext_insert_index(inode_ref, path, at - 1, insert_index,
				    npath[0].block.lb_id, *ins_right_leaf);

cleanup:
	if (ret != EOK) {
		if (newblock)
			ext4_ext_free_blocks(inode_ref, newblock, 1, 0);

		npath_at = depth - at;
		while (npath_at >= 0) {
			if (npath[npath_at].block.lb_id) {
				newblock = npath[npath_at].block.lb_id;
				bufRelease(npath[npath_at].block.buf);
				ext4_ext_free_blocks(inode_ref, newblock, 1, 0);
				memset(&npath[npath_at].block, 0,
				       sizeof(struct ext4_block));
			}
			npath_at--;
		}
	}
	return ret;
}

static inline void ext4_ext_replace_path(struct ext4_inode_ref *inode_ref,
					 struct ext4_extent_path *path,
					 struct ext4_extent_path *newpath,
					 int at)
{
	ext4_ext_drop_refs(inode_ref, path + at, 1);
	path[at] = *newpath;
	memset(newpath, 0, sizeof(struct ext4_extent_path));
}


int ext4_ext_insert_extent(struct ext4_inode_ref *inode_ref,
			   struct ext4_extent_path **ppath,
			   struct ext4_extent *newext, int flags)
{
	int depth, level = 0, ret = 0;
	struct ext4_extent_path *path = *ppath;
	struct ext4_extent_path *npath = NULL;
	bool ins_right_leaf = false;
	bool need_split;
	int EOK = 0;
again:
	depth = ext_depth(inode_ref->inode);
	ret = ext4_ext_insert_leaf(inode_ref, path, depth, newext, flags,
				   &need_split);
	if (ret == -1 && need_split == true) {
		int i;
		for (i = depth, level = 0; i >= 0; i--, level++)
			if (EXT_HAS_FREE_INDEX(path + i))
				break;

		/* Do we need to grow the tree? */
		if (i < 0) {
			ret = ext4_ext_grow_indepth(inode_ref, 0);
			if (ret != EOK)
				goto out;

			ret = ext4_find_extent(
			    inode_ref, to_le32(newext->first_block), ppath, 0);
			if (ret != EOK)
				goto out;

			path = *ppath;
			/*
			 * After growing the tree, there should be free space in
			 * the only child node of the root.
			 */
			level--;
			depth++;
		}

		i = depth - (level - 1);
		/* We split from leaf to the i-th node */
		if (level > 0) {
			npath = ext4_calloc(1, sizeof(struct ext4_extent_path) *
					      (level));
			if (!npath) {
				ret = -1;
				goto out;
			}
			ret = ext4_ext_split_node(inode_ref, path, i, newext,
						  npath, &ins_right_leaf);
			if (ret != EOK)
				goto out;

			while (--level >= 0) {
				if (ins_right_leaf)
					ext4_ext_replace_path(inode_ref, path,
							      &npath[level],
							      i + level);
				else if (npath[level].block.lb_id)
					ext4_ext_drop_refs(inode_ref,
							   npath + level, 1);
			}
		}
		goto again;
	}

out:
	if (ret != EOK) {
		if (path)
			ext4_ext_drop_refs(inode_ref, path, 0);

		while (--level >= 0 && npath) {
			if (npath[level].block.lb_id) {
				ext4_fsblk_t block = npath[level].block.lb_id;
				ext4_ext_free_blocks(inode_ref, block, 1, 0);
				ext4_ext_drop_refs(inode_ref, npath + level, 1);
			}
		}
	}
	if (npath)
		ext4_free(npath);

	return ret;
}

static int ext4_ext_split_extent_at(struct ext4_inode_ref *inode_ref,
				    struct ext4_extent_path **ppath,
				    ext4_lblk_t split, uint32_t split_flag)
{
	int EOK = 0;
	struct ext4_extent *ex, newex;
	ext4_fsblk_t newblock;
	ext4_lblk_t ee_block;
	int32_t ee_len;
	int32_t depth = ext_depth(inode_ref->inode);
	int err = EOK;

	ex = (*ppath)[depth].extent;
	ee_block = to_le32(ex->first_block);
	ee_len = ext4_ext_get_actual_len(ex);
	newblock = split - ee_block + ext4_ext_pblock(ex);

	if (split == ee_block) {
        /*
         * case b: block @split 是范围开始的块
         * 然后我们只改变范围的状态，并分裂
         * 不需要。
         */
		if (split_flag & EXT4_EXT_MARK_UNWRIT2)
			ext4_ext_mark_unwritten(ex);
		else
			ext4_ext_mark_initialized(ex);
		err = ext4_ext_dirty(inode_ref, *ppath + depth);
		goto out;
	}

	ex->block_count = to_le16(split - ee_block);
	if (split_flag & EXT4_EXT_MARK_UNWRIT1)
		ext4_ext_mark_unwritten(ex);

	err = ext4_ext_dirty(inode_ref, *ppath + depth);
	if (err != EOK)
		goto out;

	newex.first_block = to_le32(split);
	newex.block_count = to_le16(ee_len - (split - ee_block));
	ext4_ext_store_pblock(&newex, newblock);
	if (split_flag & EXT4_EXT_MARK_UNWRIT2)
		ext4_ext_mark_unwritten(&newex);
	err = ext4_ext_insert_extent(inode_ref, ppath, &newex,
				     EXT4_EXT_NO_COMBINE);
	if (err != EOK)
		goto restore_extent_len;

out:
	return err;
restore_extent_len:
	ex->block_count = to_le16(ee_len);
	err = ext4_ext_dirty(inode_ref, *ppath + depth);
	return err;
}

static int ext4_ext_convert_to_initialized(struct ext4_inode_ref *inode_ref,
					   struct ext4_extent_path **ppath,
					   ext4_lblk_t split, uint32_t blocks)
{
	int EOK = 0;
	int32_t depth = ext_depth(inode_ref->inode), err = EOK;
	struct ext4_extent *ex = (*ppath)[depth].extent;

	ASSERT(to_le32(ex->first_block) <= split);

	if (split + blocks ==
	    to_le32(ex->first_block) + ext4_ext_get_actual_len(ex)) {
		/* split and initialize right part */
		err = ext4_ext_split_extent_at(inode_ref, ppath, split,
					       EXT4_EXT_MARK_UNWRIT1);
	} else if (to_le32(ex->first_block) == split) {
		/* split and initialize left part */
		err = ext4_ext_split_extent_at(inode_ref, ppath, split + blocks,
					       EXT4_EXT_MARK_UNWRIT2);
	} else {
		/* split 1 extent to 3 and initialize the 2nd */
		err = ext4_ext_split_extent_at(inode_ref, ppath, split + blocks,
					       EXT4_EXT_MARK_UNWRIT1 |
						   EXT4_EXT_MARK_UNWRIT2);
		if (err == EOK) {
			err = ext4_ext_split_extent_at(inode_ref, ppath, split,
						       EXT4_EXT_MARK_UNWRIT1);
		}
	}

	return err;
}

static ext4_fsblk_t ext4_ext_find_goal(struct ext4_inode_ref *inode_ref,
				       struct ext4_extent_path *path,
				       ext4_lblk_t block)
{
	if (path) {
		uint32_t depth = path->depth;
		struct ext4_extent *ex;

		/*
		 * Try to predict block placement assuming that we are
		 * filling in a file which will eventually be
		 * non-sparse --- i.e., in the case of libbfd writing
		 * an ELF object sections out-of-order but in a way
		 * the eventually results in a contiguous object or
		 * executable file, or some database extending a table
		 * space file.  However, this is actually somewhat
		 * non-ideal if we are writing a sparse file such as
		 * qemu or KVM writing a raw image file that is going
		 * to stay fairly sparse, since it will end up
		 * fragmenting the file system's free space.  Maybe we
		 * should have some hueristics or some way to allow
		 * userspace to pass a hint to file system,
		 * especially if the latter case turns out to be
		 * common.
		 */
		ex = path[depth].extent;
		if (ex) {
			ext4_fsblk_t ext_pblk = ext4_ext_pblock(ex);
			ext4_lblk_t ext_block = to_le32(ex->first_block);

			if (block > ext_block)
				return ext_pblk + (block - ext_block);
			else
				return ext_pblk - (ext_block - block);
		}

		/* it looks like index is empty;
		 * try to find starting block from index itself */
		if (path[depth].block.lb_id)
			return path[depth].block.lb_id;
	}

	/* OK. use inode's group */
	return ext4_fs_inode_to_goal_block(inode_ref);
}




static int ext4_ext_zero_unwritten_range(struct ext4_inode_ref *inode_ref,
					 ext4_fsblk_t block,
					 uint32_t blocks_count)
{
	int EOK = 0;
	int err = EOK;
	uint32_t i;
	uint32_t block_size = ext4_sb_get_block_size(&inode_ref->fs->superBlock.ext4_sblock);
	for (i = 0; i < blocks_count; i++) {
		struct ext4_block bh = EXT4_BLOCK_ZERO();
		bh.buf = bufRead(1,EXT4_LBA2PBA(block + i),1);
		if (bh.buf == NULL)
		{
			err = -1;
			break;
		}
		memset(bh.buf->data->data, 0, block_size);
		bh.buf->dirty = 1;
		bufRelease(bh.buf);
	}
	return err;
}

static ext4_lblk_t ext4_ext_next_allocated_block(struct ext4_extent_path *path)
{
	int32_t depth;

	depth = path->depth;

	if (depth == 0 && path->extent == NULL)
		return -1;

	while (depth >= 0) {
		if (depth == path->depth) {
			/* leaf */
			if (path[depth].extent &&
			    path[depth].extent !=
				EXT_LAST_EXTENT(path[depth].header))
				return to_le32(
				    path[depth].extent[1].first_block);
		} else {
			/* index */
			if (path[depth].index !=
			    EXT_LAST_INDEX(path[depth].header))
				return to_le32(
				    path[depth].index[1].first_block);
		}
		depth--;
	}

	return -1;
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
