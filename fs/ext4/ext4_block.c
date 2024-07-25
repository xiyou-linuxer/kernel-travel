#include <fs/fs.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <debug.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/mount.h>
#include <fs/buf.h>
#include <fs/ext4.h>
#include <fs/ext4_sb.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_bitmap.h>
#include <fs/ext4_config.h>
#include <fs/ext4_crc32.h>
#include <fs/ext4_block_group.h>

/**
 * @brief 读取连续的扇区
 * @param buf 缓冲区
 * @param block_size 逻辑数据块的大小
 * @param lba 其实扇区号
 * @param cnt 连续读取的数量
 */

int ext4_blocks_get_direct(const void *buf,int block_size, uint64_t lba, uint32_t cnt)
{
	int count = 0;
	uint8_t* p = (void*)buf;
	int lp = block_size/BUF_SIZE;
	while (count < cnt)
	{
		int pba = lp*(lba+count);//计算逻辑块号
		for (int i = 0; i < lp; i++)//将块内的扇区都读出来
		{
			Buffer *buf = bufRead(1,pba+i,1);
			memcpy(p,buf->data->data,BUF_SIZE);
			p+=BUF_SIZE;
		}
		count++;
	}
	return count;
}

/**
 * @brief 写取连续的扇区
 * @param 缓冲区 超级块指针。
 * @param lba 其实扇区号
 * @param cnt 连续读取的数量
 */

int ext4_blocks_set_direct(const void *buf,int block_size,uint64_t lba, uint32_t cnt)
{
	int count = 0;
	uint8_t* p = (void*)buf;
	int lp = block_size/BUF_SIZE;
	while (count < cnt)
	{
		int pba = lp*(lba+count);
		for (int i = 0; i < 8; i++)
		{
			Buffer *buf = bufRead(1,pba+i,0);//只获取缓冲区，读取其中内容
			memcpy(buf->data->data,p,BUF_SIZE);
			bufWrite(buf);
			p+=BUF_SIZE;
		}
		
		count++;
	}
	return count;
}

int ext4_trans_set_block_dirty(struct Buffer *buf)
{
    buf->dirty = 1;
    return 0;
}

uint32_t ext4_balloc_get_bgid_of_block(struct ext4_sblock *s,
				       uint64_t baddr)
{
	if (ext4_get32(s, first_data_block) && baddr)
		baddr--;

	return (uint32_t)(baddr / ext4_get32(s, blocks_per_group));
}

/**
 * @brief 计算块组的起始块地址
 * @param s 超级块指针。
 * @param bgid 块组索引
 * @return 区块地址
 */
uint64_t ext4_balloc_get_block_of_bgid(struct ext4_sblock *s,uint32_t bgid)
{
	uint64_t baddr = 0;
	if (ext4_get32(s, first_data_block))
		baddr++;

	baddr += bgid * ext4_get32(s, blocks_per_group);
	return baddr;
}

#define ext4_balloc_verify_bitmap_csum(...) true

#if CONFIG_META_CSUM_ENABLE
static uint32_t ext4_balloc_bitmap_csum(struct ext4_sblock *sb, void *bitmap)
{
	uint32_t checksum = 0;
	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM)) {
		uint32_t blocks_per_group = ext4_get32(sb, blocks_per_group);

		/* First calculate crc32 checksum against fs uuid */
		checksum = ext4_crc32c(EXT4_CRC32_INIT, sb->uuid,
				sizeof(sb->uuid));
		/* Then calculate crc32 checksum against block_group_desc */
		checksum = ext4_crc32c(checksum, bitmap, blocks_per_group / 8);
	}
	return checksum;
}
#else
#define ext4_balloc_bitmap_csum(...) 0
#endif

void ext4_balloc_set_bitmap_csum(struct ext4_sblock *sb, struct ext4_bgroup *bg, void *bitmap __attribute__ ((__unused__)))
{
	int desc_size = ext4_sb_get_desc_size(sb);
	uint32_t checksum = ext4_balloc_bitmap_csum(sb, bitmap);
	uint16_t lo_checksum = to_le16(checksum & 0xFFFF),
		 hi_checksum = to_le16(checksum >> 16);

	if (!ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM))
		return;

	/* See if we need to assign a 32bit checksum */
	bg->block_bitmap_csum_lo = lo_checksum;
	if (desc_size == EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->block_bitmap_csum_hi = hi_checksum;

}

int ext4_balloc_alloc_block(struct ext4_inode_ref *inode_ref, ext4_fsblk_t goal, ext4_fsblk_t *fblock)
{
	int EOK = 0;
	ext4_fsblk_t alloc = 0;
	ext4_fsblk_t bmp_blk_adr;
	uint32_t rel_blk_idx = 0;
	uint64_t free_blocks;
	int r;

	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;

	/* 获取目标块所在的块组号和相对索引 */
	uint32_t bg_id = ext4_balloc_get_bgid_of_block(sb, goal);
	uint32_t idx_in_bg = ext4_fs_addr_to_idx_bg(sb, goal);

	struct ext4_block b;
	struct ext4_block_group_ref bg_ref;

	/* 获取块组引用 */
	r = ext4_fs_get_block_group_ref(inode_ref->fs, bg_id, &bg_ref);
	if (r != EOK)
		return r;

	struct ext4_bgroup *bg = bg_ref.block_group;

	free_blocks = ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
	if (free_blocks == 0) {
		/* 此块组没有空闲块 */
		goto goal_failed;
	}

	/* 计算索引 */
	ext4_fsblk_t first_in_bg;
	first_in_bg = ext4_balloc_get_block_of_bgid(sb, bg_ref.index);

	uint32_t first_in_bg_index;
	first_in_bg_index = ext4_fs_addr_to_idx_bg(sb, first_in_bg);

	if (idx_in_bg < first_in_bg_index)
		idx_in_bg = first_in_bg_index;

	/* 加载包含位图的块 */
	bmp_blk_adr = ext4_bg_get_block_bitmap(bg_ref.block_group, sb);

	b.buf = bufRead(1,EXT4_LBA2PBA(bmp_blk_adr),1);
	if (r != EOK) {
		ext4_fs_put_block_group_ref(&bg_ref);
		return r;
	}

	if (!ext4_balloc_verify_bitmap_csum(sb, bg, b.data)) {
		printk("Bitmap checksum failed, Group: %d \n", bg_ref.index);
	}

	/* 检查目标是否为空闲 */
	if (ext4_bmap_is_bit_clr((uint8_t *)b.buf->data->data, idx_in_bg)) {
		ext4_bmap_bit_set((uint8_t *)b.buf->data->data, idx_in_bg);
		ext4_balloc_set_bitmap_csum(sb, bg_ref.block_group, b.buf->data->data);
		bufWrite(b.buf);
		bufRelease(b.buf);
		alloc = ext4_fs_bg_idx_to_addr(sb, idx_in_bg, bg_id);
		goto success;
	}

	uint32_t blk_in_bg = ext4_blocks_in_group_cnt(sb, bg_id);

	uint32_t end_idx = (idx_in_bg + 63) & ~63;
	if (end_idx > blk_in_bg)
		end_idx = blk_in_bg;

	/* 尝试在目标附近找到空闲块 */
	uint32_t tmp_idx;
	for (tmp_idx = idx_in_bg + 1; tmp_idx < end_idx; ++tmp_idx) {
		if (ext4_bmap_is_bit_clr((uint8_t *)b.buf->data->data, tmp_idx)) {
			ext4_bmap_bit_set((uint8_t *)b.buf->data->data, tmp_idx);
			ext4_balloc_set_bitmap_csum(sb, bg, b.buf->data->data);
			bufWrite(b.buf);
			bufRelease(b.buf);
			alloc = ext4_fs_bg_idx_to_addr(sb, tmp_idx, bg_id);
			goto success;
		}
	}

	/* 在位图中查找空闲位 */
	r = ext4_bmap_bit_find_clr((uint8_t *)b.buf->data->data, idx_in_bg, blk_in_bg, &rel_blk_idx);
	if (r == EOK) {
		ext4_bmap_bit_set((uint8_t *)b.buf->data->data, rel_blk_idx);
		ext4_balloc_set_bitmap_csum(sb, bg_ref.block_group, b.buf->data->data);
		bufWrite(b.buf);
		bufRelease(b.buf);
		alloc = ext4_fs_bg_idx_to_addr(sb, rel_blk_idx, bg_id);
		goto success;
	}

	/* 尚未找到空闲块 */
	bufRelease(b.buf);
	if (r != EOK) {
		ext4_fs_put_block_group_ref(&bg_ref);
		return r;
	}

goal_failed:

	r = ext4_fs_put_block_group_ref(&bg_ref);
	if (r != EOK)
		return r;

	/* 尝试其他块组 */
	uint32_t block_group_count = ext4_block_group_cnt(sb);
	uint32_t bgid = (bg_id + 1) % block_group_count;
	uint32_t count = block_group_count;

	while (count > 0) {
		r = ext4_fs_get_block_group_ref(inode_ref->fs, bgid, &bg_ref);
		if (r != EOK)
			return r;

		struct ext4_bgroup *bg = bg_ref.block_group;
		free_blocks = ext4_bg_get_free_blocks_count(bg, sb);
		if (free_blocks == 0) {
			/* 此块组没有空闲块 */
			goto next_group;
		}

		/* 加载包含位图的块 */
		bmp_blk_adr = ext4_bg_get_block_bitmap(bg, sb);
		bufRead(1,EXT4_LBA2PBA(bmp_blk_adr),1);

		if (!ext4_balloc_verify_bitmap_csum(sb, bg, b.buf->data.\)) {
			printk("Bitmap checksum failed, Group: %d \n", bg_ref.index);
		}

		/* 计算索引 */
		first_in_bg = ext4_balloc_get_block_of_bgid(sb, bgid);
		idx_in_bg = ext4_fs_addr_to_idx_bg(sb, first_in_bg);
		blk_in_bg = ext4_blocks_in_group_cnt(sb, bgid);
		first_in_bg_index = ext4_fs_addr_to_idx_bg(sb, first_in_bg);

		if (idx_in_bg < first_in_bg_index)
			idx_in_bg = first_in_bg_index;

		r = ext4_bmap_bit_find_clr((uint8_t *)b.buf->data->data, idx_in_bg, blk_in_bg,
				&rel_blk_idx);
		if (r == EOK) {
			ext4_bmap_bit_set((uint8_t *)b.buf->data->data, rel_blk_idx);
			ext4_balloc_set_bitmap_csum(sb, bg, b.buf->data->data);
			bufWrite(b.buf);
			bufRelease(b.buf);
			alloc = ext4_fs_bg_idx_to_addr(sb, rel_blk_idx, bgid);
			goto success;//找到可用的位置
		}

		bufRelease(b.buf);
		if (r != EOK) {
			ext4_fs_put_block_group_ref(&bg_ref);
			return r;
		}

	next_group:
		r = ext4_fs_put_block_group_ref(&bg_ref);
		if (r != EOK) {
			return r;
		}

		/* 转到下一个组 */
		bgid = (bgid + 1) % block_group_count;
		count--;
	}
	//遍历所有块组都未找到则返回错误
	return -1;

success:

	uint32_t block_size = ext4_sb_get_block_size(sb);

	/* 更新超级块的空闲块数 */
	uint64_t sb_free_blocks = ext4_sb_get_free_blocks_cnt(sb);
	sb_free_blocks--;
	ext4_sb_set_free_blocks_cnt(sb, sb_free_blocks);

	/* 更新inode的块数（不同的块大小！） */
	uint64_t ino_blocks = ext4_inode_get_blocks_count(sb, inode_ref->inode);
	ino_blocks += block_size / EXT4_INODE_BLOCK_SIZE;
	ext4_inode_set_blocks_count(sb, inode_ref->inode, ino_blocks);
	inode_ref->dirty = true;

	/* 更新块组的空闲块数 */
	uint32_t fb_cnt = ext4_bg_get_free_blocks_count(bg_ref.block_group, sb);
	fb_cnt--;
	ext4_bg_set_free_blocks_count(bg_ref.block_group, sb, fb_cnt);

	bg_ref.dirty = true;
	r = ext4_fs_put_block_group_ref(&bg_ref);

	*fblock = alloc;
	return r;
}

int ext4_balloc_free_blocks(struct ext4_inode_ref *inode_ref,
			    ext4_fsblk_t first, uint32_t count)
{
	int EOK = 0;
	int rc = EOK;
	uint32_t blk_cnt = count; // 要释放的块数
	ext4_fsblk_t start_block = first; // 第一个要释放的块
	struct FileSystem *fs = inode_ref->fs;
	struct ext4_sblock *sb = &fs->superBlock.ext4_sblock;

	/* 计算第一个和最后一个块所在的块组 */
	uint32_t bg_first = ext4_balloc_get_bgid_of_block(sb, first);
	uint32_t bg_last = ext4_balloc_get_bgid_of_block(sb, first + count - 1);

	if (!ext4_sb_feature_incom(sb, EXT4_FINCOM_FLEX_BG)) {
		/* 如果未启用FLEX_BG特性，确保所有块在同一块组内 */
		if (bg_last != bg_first) {
			printk("FLEX_BG: disabled & bg_last: %d  bg_first: %d\n", bg_last, bg_first);
		}
	}

	/* 循环处理从第一个块组到最后一个块组 */
	struct ext4_block_group_ref bg_ref;
	while (bg_first <= bg_last) {

		/* 获取块组引用 */
		rc = ext4_fs_get_block_group_ref(fs, bg_first, &bg_ref);
		if (rc != EOK)
			return rc;

		struct ext4_bgroup *bg = bg_ref.block_group;

		uint32_t idx_in_bg_first;
		idx_in_bg_first = ext4_fs_addr_to_idx_bg(sb, first);

		/* 加载包含位图的块 */
		ext4_fsblk_t bitmap_blk = ext4_bg_get_block_bitmap(bg, sb);

		struct ext4_block blk;
		bufRead(1,EXT4_LBA2PBA(bitmap_blk) ,1);
		if (rc != EOK) {
			ext4_fs_put_block_group_ref(&bg_ref);
			return rc;
		}

		/* 验证位图校验和 */
		if (!ext4_balloc_verify_bitmap_csum(sb, bg, blk.buf->data->data)) {
			printk("位图校验和失败.组: %d\n",bg_ref.index);
		}

		/* 计算要释放的块数 */
		uint32_t free_cnt;
		free_cnt = ext4_sb_get_block_size(sb) * 8 - idx_in_bg_first;

		/* 如果是最后一个块组，只释放count指定的块数 */
		free_cnt = count > free_cnt ? free_cnt : count;

		/* 修改位图 */
		ext4_bmap_bits_free(blk.buf->data->data, idx_in_bg_first, free_cnt);
		ext4_balloc_set_bitmap_csum(sb, bg, blk.buf->data->data);
		blk.buf->dirty = 1;
		ext4_trans_set_block_dirty(blk.buf);

		/* 更新计数和索引 */
		count -= free_cnt;
		first += free_cnt;

		/* 释放包含位图的块 */
		bufRelease(blk.buf);

		uint32_t block_size = ext4_sb_get_block_size(sb);

		/* 更新超级块的空闲块数 */
		uint64_t sb_free_blocks = ext4_sb_get_free_blocks_cnt(sb);
		sb_free_blocks += free_cnt;
		ext4_sb_set_free_blocks_cnt(sb, sb_free_blocks);

		/* 更新inode的块数 */
		uint64_t ino_blocks;
		ino_blocks = ext4_inode_get_blocks_count(sb, inode_ref->inode);
		ino_blocks -= free_cnt * (block_size / EXT4_INODE_BLOCK_SIZE);
		ext4_inode_set_blocks_count(sb, inode_ref->inode, ino_blocks);
		inode_ref->dirty = true;

		/* 更新块组的空闲块数 */
		uint32_t free_blocks;
		free_blocks = ext4_bg_get_free_blocks_count(bg, sb);
		free_blocks += free_cnt;
		ext4_bg_set_free_blocks_count(bg, sb, free_blocks);
		bg_ref.dirty = true;

		/* 释放块组引用 */
		rc = ext4_fs_put_block_group_ref(&bg_ref);
		if (rc != EOK)
			break;

		/* 处理下一个块组 */
		bg_first++;
	}

	/* 使缓存中的块无效 */
	//ext4_bcache_invalidate_lba(fs->bdev->bc, start_block, blk_cnt);

	/* 确保所有块都已释放 */
	ASSERT(count == 0);

	return rc;
}

int ext4_block_readbytes(uint64_t off, void *buf, uint32_t len)
{
	uint64_t block_idx;
	uint32_t blen;
	uint32_t unalg;
	int r = 0;
	int ph_bsize = 512;
	uint8_t* p = (void*)buf;

	// 计算起始块索引
	block_idx = (off /BUF_SIZE);
	// 处理第一个未对齐的块
	unalg = (off & (ph_bsize - 1));
	if (unalg) {

		// 计算读取长度
		uint32_t rlen = (ph_bsize - unalg) > len ? len : (ph_bsize - unalg);

		// 读取整个块到临时缓冲区
		Buffer *buffer = bufRead(1,block_idx,1);
		if (r != 0)
			return r;

		// 将数据复制到目标缓冲区
		memcpy(p, buffer->data->data + unalg, rlen);

		// 更新指针和剩余长度
		p += rlen;
		len -= rlen;
		block_idx++;
	}

	// 处理对齐的数据
	blen = len / ph_bsize;
	if (blen != 0) {
		// 读取对齐的块
		int count = 0;
		while (count < blen)
		{
			Buffer *buffer = bufRead(1,block_idx, 1);
			memcpy(p, buffer->data->data, ph_bsize);
			p += ph_bsize;
			len -= ph_bsize;
			block_idx++;
			count++;
		}
	}
	// 处理剩余的数据
	if (len) {
		// 读取最后一个块到临时缓冲区
		Buffer *buffer = bufRead(1,block_idx, 1);
		if (r != 0)
			return r;

		// 将数据复制到目标缓冲区
		memcpy(p, buffer->data->data + unalg, len);
	}
	return r;
}

int ext4_block_writebytes(uint64_t off, const void *buf, uint32_t len)
{
	uint64_t block_idx;
	uint32_t blen;
	uint32_t unalg;
	int r = 0;
	int ph_bsize = 512;
	const uint8_t *p = (void *)buf;

	block_idx = (off / ph_bsize);

	/*OK lets deal with the first possible unaligned block*/
	unalg = (off & (ph_bsize - 1));
	if (unalg) {
		uint32_t wlen = (ph_bsize - unalg) > len ? len : (ph_bsize - unalg);
		Buffer *buffer = bufRead(1,block_idx,1);
		memcpy(buffer->data->data + unalg, p, wlen);
		bufWrite(buffer);
		p += wlen;
		len -= wlen;
		block_idx++;
	}

	/*Aligned data*/
	blen = len / ph_bsize;
	if (blen != 0) {
		int count = 0;
		while (count < blen)
		{
			Buffer *buffer = bufRead(1,block_idx ,1);
			memcpy(buffer->data->data,p,ph_bsize);
			p+=ph_bsize;
			len -= ph_bsize;
			block_idx++;
			count++;
		}
	}

	/*Rest of the data*/
	if (len) {
		Buffer *buffer = bufRead(1,block_idx ,1);

		memcpy(buffer->data->data, p, len);

		bufWrite(buffer);
	}

	return r;
}