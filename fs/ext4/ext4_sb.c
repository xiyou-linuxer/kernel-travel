#include <fs/ext4.h>
#include <ext4_sb.h>
#include <ext4_crc32.h>

uint32_t ext4_block_group_cnt(struct ext4_sblock *s)
{
	uint64_t blocks_count = ext4_sb_get_blocks_cnt(s);
	uint32_t blocks_per_group = ext4_get32(s, blocks_per_group);

	uint32_t block_groups_count = (uint32_t)(blocks_count / blocks_per_group);

	if (blocks_count % blocks_per_group)
		block_groups_count++;

	return block_groups_count;
}

uint32_t ext4_blocks_in_group_cnt(struct ext4_sblock *s, uint32_t bgid)
{
	uint32_t block_group_count = ext4_block_group_cnt(s);
	uint32_t blocks_per_group = ext4_get32(s, blocks_per_group);
	uint64_t total_blocks = ext4_sb_get_blocks_cnt(s);

	if (bgid < block_group_count - 1)
		return blocks_per_group;

	return (uint32_t)(total_blocks - ((block_group_count - 1) * blocks_per_group));
}

uint32_t ext4_inodes_in_group_cnt(struct ext4_sblock *s, uint32_t bgid)
{
	uint32_t block_group_count = ext4_block_group_cnt(s);
	uint32_t inodes_per_group = ext4_get32(s, inodes_per_group);
	uint32_t total_inodes = ext4_get32(s, inodes_count);

	if (bgid < block_group_count - 1)
		return inodes_per_group;

	return (total_inodes - ((block_group_count - 1) * inodes_per_group));
}

#if CONFIG_META_CSUM_ENABLE
static uint32_t ext4_sb_csum(struct ext4_sblock *s)
{

	return ext4_crc32c(EXT4_CRC32_INIT, s,
			offsetof(struct ext4_sblock, checksum));
}
#else
#define ext4_sb_csum(...) 0
#endif

static bool ext4_sb_verify_csum(struct ext4_sblock *s)
{
	if (!ext4_sb_feature_ro_com(s, EXT4_FRO_COM_METADATA_CSUM))
		return true;

	if (s->checksum_type != to_le32(EXT4_CHECKSUM_CRC32C))
		return false;

	return s->checksum == to_le32(ext4_sb_csum(s));
}

static void ext4_sb_set_csum(struct ext4_sblock *s)
{
	if (!ext4_sb_feature_ro_com(s, EXT4_FRO_COM_METADATA_CSUM))
		return;

	s->checksum = to_le32(ext4_sb_csum(s));
}

int ext4_sb_write(struct ext4_blockdev *bdev, struct ext4_sblock *s)
{
	ext4_sb_set_csum(s);
	//return ext4_block_writebytes(bdev, EXT4_SUPERBLOCK_OFFSET, s,EXT4_SUPERBLOCK_SIZE);
}

int ext4_sb_read(struct ext4_blockdev *bdev, struct ext4_sblock *s)
{
	//return ext4_block_readbytes(bdev, EXT4_SUPERBLOCK_OFFSET, s, EXT4_SUPERBLOCK_SIZE);
}

bool ext4_sb_check(struct ext4_sblock *s)
{
	if (ext4_get16(s, magic) != EXT4_SUPERBLOCK_MAGIC)
		return false;

	if (ext4_get32(s, inodes_count) == 0)
		return false;

	if (ext4_sb_get_blocks_cnt(s) == 0)
		return false;

	if (ext4_get32(s, blocks_per_group) == 0)
		return false;

	if (ext4_get32(s, inodes_per_group) == 0)
		return false;

	if (ext4_get16(s, inode_size) < 128)
		return false;

	if (ext4_get32(s, first_inode) < 11)
		return false;

	if (ext4_sb_get_desc_size(s) < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return false;

	if (ext4_sb_get_desc_size(s) > EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return false;

	if (!ext4_sb_verify_csum(s))
		return false;

	return true;
}

static inline int is_power_of(uint32_t a, uint32_t b)
{
	while (1) {
		if (a < b)
			return 0;
		if (a == b)
			return 1;
		if ((a % b) != 0)
			return 0;
		a = a / b;
	}
}

bool ext4_sb_sparse(uint32_t group)
{
	if (group <= 1)
		return 1;

	if (!(group & 1))
		return 0;

	return (is_power_of(group, 7) || is_power_of(group, 5) ||
		is_power_of(group, 3));
}

bool ext4_sb_is_super_in_bg(struct ext4_sblock *s, uint32_t group)
{
	if (ext4_sb_feature_ro_com(s, EXT4_FRO_COM_SPARSE_SUPER) &&
	    !ext4_sb_sparse(group))
		return false;
	return true;
}

static uint32_t ext4_bg_num_gdb_meta(struct ext4_sblock *s, uint32_t group)
{
	uint32_t dsc_per_block =
	    ext4_sb_get_block_size(s) / ext4_sb_get_desc_size(s);

	uint32_t metagroup = group / dsc_per_block;
	uint32_t first = metagroup * dsc_per_block;
	uint32_t last = first + dsc_per_block - 1;

	if (group == first || group == first + 1 || group == last)
		return 1;
	return 0;
}

static uint32_t ext4_bg_num_gdb_nometa(struct ext4_sblock *s, uint32_t group)
{
	if (!ext4_sb_is_super_in_bg(s, group))
		return 0;
	uint32_t dsc_per_block =
	    ext4_sb_get_block_size(s) / ext4_sb_get_desc_size(s);

	uint32_t db_count =
	    (ext4_block_group_cnt(s) + dsc_per_block - 1) / dsc_per_block;

	if (ext4_sb_feature_incom(s, EXT4_FINCOM_META_BG))
		return ext4_sb_first_meta_bg(s);

	return db_count;
}

uint32_t ext4_bg_num_gdb(struct ext4_sblock *s, uint32_t group)
{
	uint32_t dsc_per_block =
	    ext4_sb_get_block_size(s) / ext4_sb_get_desc_size(s);
	uint32_t first_meta_bg = ext4_sb_first_meta_bg(s);
	uint32_t metagroup = group / dsc_per_block;

	if (!ext4_sb_feature_incom(s,EXT4_FINCOM_META_BG) ||
	    metagroup < first_meta_bg)
		return ext4_bg_num_gdb_nometa(s, group);

	return ext4_bg_num_gdb_meta(s, group);
}

uint32_t ext4_num_base_meta_clusters(struct ext4_sblock *s,
				     uint32_t block_group)
{
	uint32_t num;
	uint32_t dsc_per_block =
	    ext4_sb_get_block_size(s) / ext4_sb_get_desc_size(s);

	num = ext4_sb_is_super_in_bg(s, block_group);

	if (!ext4_sb_feature_incom(s, EXT4_FINCOM_META_BG) ||
	    block_group < ext4_sb_first_meta_bg(s) * dsc_per_block) {
		if (num) {
			num += ext4_bg_num_gdb(s, block_group);
			num += ext4_get16(s, s_reserved_gdt_blocks);
		}
	} else {
		num += ext4_bg_num_gdb(s, block_group);
	}

	uint32_t clustersize = 1024 << ext4_get32(s, log_cluster_size);
	uint32_t cluster_ratio = clustersize / ext4_sb_get_block_size(s);
	uint32_t v =
	    (num + cluster_ratio - 1) >> ext4_get32(s, log_cluster_size);

	return v;
}
