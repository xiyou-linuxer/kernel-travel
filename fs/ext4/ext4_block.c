#include <fs/fs.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/mount.h>
#include <fs/buf.h>
#include <fs/ext4.h>
#include <fs/ext4_sb.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_dir.h>
int ext4_balloc_alloc_block(struct ext4_inode_ref *inode_ref,
			    ext4_fsblk_t goal,
			    ext4_fsblk_t *fblock)
{
	ext4_fsblk_t alloc = 0;
	ext4_fsblk_t bmp_blk_adr;
	uint32_t rel_blk_idx = 0;
	uint64_t free_blocks;
	int r;
	struct ext4_sblock *sb = &inode_ref->fs->sb;

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

	r = ext4_trans_block_get(inode_ref->fs->bdev, &b, bmp_blk_adr);
	if (r != EOK) {
		ext4_fs_put_block_group_ref(&bg_ref);
		return r;
	}

	if (!ext4_balloc_verify_bitmap_csum(sb, bg, b.data)) {
		ext4_dbg(DEBUG_BALLOC,
			DBG_WARN "位图校验和失败."
			"组: %" PRIu32"\n",
			bg_ref.index);
	}

	/* 检查目标是否为空闲 */
	if (ext4_bmap_is_bit_clr(b.data, idx_in_bg)) {
		ext4_bmap_bit_set(b.data, idx_in_bg);
		ext4_balloc_set_bitmap_csum(sb, bg_ref.block_group,
					    b.data);
		ext4_trans_set_block_dirty(b.buf);
		r = ext4_block_set(inode_ref->fs->bdev, &b);
		if (r != EOK) {
			ext4_fs_put_block_group_ref(&bg_ref);
			return r;
		}

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
		if (ext4_bmap_is_bit_clr(b.data, tmp_idx)) {
			ext4_bmap_bit_set(b.data, tmp_idx);

			ext4_balloc_set_bitmap_csum(sb, bg, b.data);
			ext4_trans_set_block_dirty(b.buf);
			r = ext4_block_set(inode_ref->fs->bdev, &b);
			if (r != EOK) {
				ext4_fs_put_block_group_ref(&bg_ref);
				return r;
			}

			alloc = ext4_fs_bg_idx_to_addr(sb, tmp_idx, bg_id);
			goto success;
		}
	}

	/* 在位图中查找空闲位 */
	r = ext4_bmap_bit_find_clr(b.data, idx_in_bg, blk_in_bg, &rel_blk_idx);
	if (r == EOK) {
		ext4_bmap_bit_set(b.data, rel_blk_idx);
		ext4_balloc_set_bitmap_csum(sb, bg_ref.block_group, b.data);
		ext4_trans_set_block_dirty(b.buf);
		r = ext4_block_set(inode_ref->fs->bdev, &b);
		if (r != EOK) {
			ext4_fs_put_block_group_ref(&bg_ref);
			return r;
		}

		alloc = ext4_fs_bg_idx_to_addr(sb, rel_blk_idx, bg_id);
		goto success;
	}

	/* 尚未找到空闲块 */
	r = ext4_block_set(inode_ref->fs->bdev, &b);
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
		r = ext4_trans_block_get(inode_ref->fs->bdev, &b, bmp_blk_adr);
		if (r != EOK) {
			ext4_fs_put_block_group_ref(&bg_ref);
			return r;
		}

		if (!ext4_balloc_verify_bitmap_csum(sb, bg, b.data)) {
			ext4_dbg(DEBUG_BALLOC,
				DBG_WARN "位图校验和失败."
				"组: %" PRIu32"\n",
				bg_ref.index);
		}

		/* 计算索引 */
		first_in_bg = ext4_balloc_get_block_of_bgid(sb, bgid);
		idx_in_bg = ext4_fs_addr_to_idx_bg(sb, first_in_bg);
		blk_in_bg = ext4_blocks_in_group_cnt(sb, bgid);
		first_in_bg_index = ext4_fs_addr_to_idx_bg(sb, first_in_bg);

		if (idx_in_bg < first_in_bg_index)
			idx_in_bg = first_in_bg_index;

		r = ext4_bmap_bit_find_clr(b.data, idx_in_bg, blk_in_bg,
				&rel_blk_idx);
		if (r == EOK) {
			ext4_bmap_bit_set(b.data, rel_blk_idx);
			ext4_balloc_set_bitmap_csum(sb, bg, b.data);
			ext4_trans_set_block_dirty(b.buf);
			r = ext4_block_set(inode_ref->fs->bdev, &b);
			if (r != EOK) {
				ext4_fs_put_block_group_ref(&bg_ref);
				return r;
			}

			alloc = ext4_fs_bg_idx_to_addr(sb, rel_blk_idx, bgid);
			goto success;
		}

		r = ext4_block_set(inode_ref->fs->bdev, &b);
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

	return ENOSPC;

success:
    /* 空命令 - 因为语法原因 */
    ;

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
	int rc = EOK;
	uint32_t blk_cnt = count; // 要释放的块数
	ext4_fsblk_t start_block = first; // 第一个要释放的块
	struct ext4_fs *fs = inode_ref->fs;
	struct ext4_sblock *sb = &fs->sb;

	/* 计算第一个和最后一个块所在的块组 */
	uint32_t bg_first = ext4_balloc_get_bgid_of_block(sb, first);
	uint32_t bg_last = ext4_balloc_get_bgid_of_block(sb, first + count - 1);

	if (!ext4_sb_feature_incom(sb, EXT4_FINCOM_FLEX_BG)) {
		/* 如果未启用FLEX_BG特性，确保所有块在同一块组内 */
		if (bg_last != bg_first) {
			ext4_dbg(DEBUG_BALLOC, DBG_WARN "FLEX_BG: disabled & "
				"bg_last: %"PRIu32" bg_first: %"PRIu32"\n",
				bg_last, bg_first);
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
		rc = ext4_trans_block_get(fs->bdev, &blk, bitmap_blk);
		if (rc != EOK) {
			ext4_fs_put_block_group_ref(&bg_ref);
			return rc;
		}

		/* 验证位图校验和 */
		if (!ext4_balloc_verify_bitmap_csum(sb, bg, blk.data)) {
			ext4_dbg(DEBUG_BALLOC,
				DBG_WARN "位图校验和失败."
				"组: %" PRIu32"\n",
				bg_ref.index);
		}

		/* 计算要释放的块数 */
		uint32_t free_cnt;
		free_cnt = ext4_sb_get_block_size(sb) * 8 - idx_in_bg_first;

		/* 如果是最后一个块组，只释放count指定的块数 */
		free_cnt = count > free_cnt ? free_cnt : count;

		/* 修改位图 */
		ext4_bmap_bits_free(blk.data, idx_in_bg_first, free_cnt);
		ext4_balloc_set_bitmap_csum(sb, bg, blk.data);
		ext4_trans_set_block_dirty(blk.buf);

		/* 更新计数和索引 */
		count -= free_cnt;
		first += free_cnt;

		/* 释放包含位图的块 */
		rc = ext4_block_set(fs->bdev, &blk);
		if (rc != EOK) {
			ext4_fs_put_block_group_ref(&bg_ref);
			return rc;
		}

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

	/* 尝试撤销块 */
	uint32_t i;
	for (i = 0; i < blk_cnt; i++) {
		rc = ext4_trans_try_revoke_block(fs->bdev, start_block + i);
		if (rc != EOK)
			return rc;
	}

	/* 使缓存中的块无效 */
	ext4_bcache_invalidate_lba(fs->bdev->bc, start_block, blk_cnt);

	/* 确保所有块都已释放 */
	ext4_assert(count == 0);

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
	block_idx = (off /ph_bsize);

	// 处理第一个未对齐的块
	unalg = (off & (ph_bsize - 1));
	if (unalg) {

		// 计算读取长度
		uint32_t rlen = (ph_bsize - unalg) > len ? len : (ph_bsize - unalg);

		// 读取整个块到临时缓冲区
		Buffer *buffer = bufRead(1, block_idx, 1);
		if (r != 0)
			return r;

		// 将数据复制到目标缓冲区
		memcpy(p, buffer->data + unalg, rlen);

		// 更新指针和剩余长度
		p += rlen;
		len -= rlen;
		block_idx++;
	}

	// 处理对齐的数据
	blen = len / ph_bsize;

	if (blen != 0) {
		// 读取对齐的块
		Buffer *buffer = bufRead(1, block_idx, 1);
		if (r != 0)
			return r;

		// 更新指针和剩余长度
		p += ph_bsize * blen;
		len -= ph_bsize * blen;

		block_idx += blen;
	}
	// 处理剩余的数据
	if (len) {
		// 读取最后一个块到临时缓冲区
		Buffer *buffer = bufRead(1, block_idx, 1);
		if (r != 0)
			return r;

		// 将数据复制到目标缓冲区
		memcpy(p, buffer->data + unalg, len);
	}
	return r;
}

int ext4_block_writebytes(struct ext4_blockdev *bdev, uint64_t off,
			  const void *buf, uint32_t len)
{
	uint64_t block_idx;
	uint32_t blen;
	uint32_t unalg;
	int r = EOK;

	const uint8_t *p = (void *)buf;

	ext4_assert(bdev && buf);

	if (!bdev->bdif->ph_refctr)
		return EIO;

	if (off + len > bdev->part_size)
		return EINVAL; /*Ups. Out of range operation*/

	block_idx = ((off + bdev->part_offset) / bdev->bdif->ph_bsize);

	/*OK lets deal with the first possible unaligned block*/
	unalg = (off & (bdev->bdif->ph_bsize - 1));
	if (unalg) {

		uint32_t wlen = (bdev->bdif->ph_bsize - unalg) > len
				    ? len
				    : (bdev->bdif->ph_bsize - unalg);

		r = ext4_bdif_bread(bdev, bdev->bdif->ph_bbuf, block_idx, 1);
		if (r != EOK)
			return r;

		memcpy(bdev->bdif->ph_bbuf + unalg, p, wlen);
		r = ext4_bdif_bwrite(bdev, bdev->bdif->ph_bbuf, block_idx, 1);
		if (r != EOK)
			return r;

		p += wlen;
		len -= wlen;
		block_idx++;
	}

	/*Aligned data*/
	blen = len / bdev->bdif->ph_bsize;
	if (blen != 0) {
		r = ext4_bdif_bwrite(bdev, p, block_idx, blen);
		if (r != EOK)
			return r;

		p += bdev->bdif->ph_bsize * blen;
		len -= bdev->bdif->ph_bsize * blen;

		block_idx += blen;
	}

	/*Rest of the data*/
	if (len) {
		r = ext4_bdif_bread(bdev, bdev->bdif->ph_bbuf, block_idx, 1);
		if (r != EOK)
			return r;

		memcpy(bdev->bdif->ph_bbuf, p, len);
		r = ext4_bdif_bwrite(bdev, bdev->bdif->ph_bbuf, block_idx, 1);
		if (r != EOK)
			return r;
	}

	return r;
}