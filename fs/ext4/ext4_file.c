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
#include <sync.h>
#include <debug.h>

int ext4_fread(Dirent *file, void *src, unsigned int off, size_t n)
{
    // 定义一些变量
    size_t* rcnt = 0;
    uint32_t unalg;
    uint32_t iblock_idx;
    uint32_t iblock_last;
    uint32_t block_size;

    ext4_fsblk_t fblock;
    ext4_fsblk_t fblock_start;
    uint32_t fblock_count;

    uint8_t *u8_buf = src;
    int r;
    struct ext4_inode_ref ref;

    // 确保文件和挂载点都存在
    ASSERT(file && file->head);

    // 如果文件是只写的，返回权限错误
    if (file->flags & O_WRONLY)
        return EPERM;

    // 如果读取大小为0，直接返回
    if (!n)
        return 0;

    struct ext4_fs *const fs = &ext4Fs->ext4_fs;
    struct ext4_sblock *const sb = &ext4Fs->superBlock.ext4_sblock;

    // 获取 inode 引用
    r = ext4_fs_get_inode_ref(fs, file->ext4_dir_en.inode, &ref);
    if (r != 0) {
        return r;
    }

    // 同步文件大小
    file->file_size = ext4_inode_get_size(sb, ref.inode);

    // 获取块大小
    block_size = ext4_sb_get_block_size(sb);
    // 调整读取大小，确保不超过文件剩余大小
    n = ((uint64_t)n > (file->file_size - off))
           ? ((size_t)(file->file_size - off))
           : n;

    // 计算块索引和未对齐的偏移量
    iblock_idx = (uint32_t)((off) / block_size);
    iblock_last = (uint32_t)((off + n) / block_size);
    unalg = (off) % block_size;

    // 如果是小于60字节的软链接
    bool softlink;
    softlink = ext4_inode_is_type(sb, ref.inode, EXT4_INODE_MODE_SOFTLINK);
    if (softlink && file->file_size < sizeof(ref.inode->blocks) &&
        !ext4_inode_get_blocks_count(sb, ref.inode)) {

        // 直接从块数组读取内容
        char *content = (char *)ref.inode->blocks;
        if (off < file->file_size) {
            size_t len = n;
            if (unalg + n > (uint32_t)file->file_size)
                len = (uint32_t)file->file_size - unalg;
            memcpy(src, content + unalg, len);
            if (rcnt)
                *rcnt = len;
        }

        r = 0;
        goto Finish;
    }

    // 处理未对齐的部分
    if (unalg) {
        size_t len = n;
        if (n > (block_size - unalg))
            len = block_size - unalg;

        // 获取数据块索引
        r = ext4_fs_get_inode_dblk_idx(&ref, iblock_idx, &fblock, true);
        if (r != 0)
            goto Finish;

        // 读取数据块内容
        if (fblock != 0) {
            uint64_t off = fblock * block_size + unalg;
            r = ext4_block_readbytes(file->mp->fs.bdev, off, u8_buf, len);
            if (r != 0)
                goto Finish;
        } else {
            // 如果是空块，填充0
            memset(u8_buf, 0, len);
        }

        // 更新缓冲区指针和剩余大小
        u8_buf += len;
        n -= len;
        off += len;

        // 更新读取计数
        if (rcnt)
            *rcnt += len;

        iblock_idx++;
    }

    // 处理对齐的部分
    fblock_start = 0;
    fblock_count = 0;
    while (n >= block_size) {
        while (iblock_idx < iblock_last) {
            // 获取数据块索引
            r = ext4_fs_get_inode_dblk_idx(&ref, iblock_idx, &fblock, true);
            if (r != EOK)
                goto Finish;

            iblock_idx++;

            if (!fblock_start)
                fblock_start = fblock;

            if ((fblock_start + fblock_count) != fblock)
                break;

            fblock_count++;
        }

        // 直接读取多个数据块
        r = ext4_blocks_get_direct(file->mp->fs.bdev, u8_buf, fblock_start, fblock_count);
        if (r != EOK)
            goto Finish;

        // 更新缓冲区指针和剩余大小
        n -= block_size * fblock_count;
        u8_buf += block_size * fblock_count;
        file->fpos += block_size * fblock_count;

        // 更新读取计数
        if (rcnt)
            *rcnt += block_size * fblock_count;

        fblock_start = fblock;
        fblock_count = 1;
    }

    // 处理剩余的部分
    if (n) {
        uint64_t off;
        r = ext4_fs_get_inode_dblk_idx(&ref, iblock_idx, &fblock, true);
        if (r != EOK)
            goto Finish;

        off = fblock * block_size;
        r = ext4_block_readbytes(file->mp->fs.bdev, off, u8_buf, n);
        if (r != EOK)
            goto Finish;

        file->fpos += n;

        // 更新读取计数
        if (rcnt)
            *rcnt += n;
    }

Finish:
    // 释放 inode 引用并解锁挂载点
    ext4_fs_put_inode_ref(&ref);
    EXT4_MP_UNLOCK(file->mp);
    return r;
}

int ext4_fwrite(ext4_file *file, const void *buf, size_t size, size_t *wcnt)
{
	uint32_t unalg;
	uint32_t iblk_idx;
	uint32_t iblock_last;
	uint32_t ifile_blocks;
	uint32_t block_size;

	uint32_t fblock_count;
	ext4_fsblk_t fblk;
	ext4_fsblk_t fblock_start;

	struct ext4_inode_ref ref;
	const uint8_t *u8_buf = buf;
	int r, rr = EOK;

	ext4_assert(file && file->mp);

	if (file->mp->fs.read_only)
		return EROFS;

	if (file->flags & O_RDONLY)
		return EPERM;

	if (!size)
		return EOK;

	EXT4_MP_LOCK(file->mp);
	ext4_trans_start(file->mp);

	struct ext4_fs *const fs = &file->mp->fs;
	struct ext4_sblock *const sb = &file->mp->fs.sb;

	if (wcnt)
		*wcnt = 0;

	r = ext4_fs_get_inode_ref(fs, file->inode, &ref);
	if (r != EOK) {
		ext4_trans_abort(file->mp);
		EXT4_MP_UNLOCK(file->mp);
		return r;
	}

	/*Sync file size*/
	file->fsize = ext4_inode_get_size(sb, ref.inode);
	block_size = ext4_sb_get_block_size(sb);

	iblock_last = (uint32_t)((file->fpos + size) / block_size);
	iblk_idx = (uint32_t)(file->fpos / block_size);
	ifile_blocks = (uint32_t)((file->fsize + block_size - 1) / block_size);

	unalg = (file->fpos) % block_size;

	if (unalg) {
		size_t len = size;
		uint64_t off;
		if (size > (block_size - unalg))
			len = block_size - unalg;

		r = ext4_fs_init_inode_dblk_idx(&ref, iblk_idx, &fblk);
		if (r != EOK)
			goto Finish;

		off = fblk * block_size + unalg;
		r = ext4_block_writebytes(file->mp->fs.bdev, off, u8_buf, len);
		if (r != EOK)
			goto Finish;

		u8_buf += len;
		size -= len;
		file->fpos += len;

		if (wcnt)
			*wcnt += len;

		iblk_idx++;
	}

	/*Start write back cache mode.*/
	r = ext4_block_cache_write_back(file->mp->fs.bdev, 1);
	if (r != EOK)
		goto Finish;

	fblock_start = 0;
	fblock_count = 0;
	while (size >= block_size) {

		while (iblk_idx < iblock_last) {
			if (iblk_idx < ifile_blocks) {
				r = ext4_fs_init_inode_dblk_idx(&ref, iblk_idx,
								&fblk);
				if (r != EOK)
					goto Finish;
			} else {
				rr = ext4_fs_append_inode_dblk(&ref, &fblk,
							       &iblk_idx);
				if (rr != EOK) {
					/* Unable to append more blocks. But
					 * some block might be allocated already
					 * */
					break;
				}
			}

			iblk_idx++;

			if (!fblock_start) {
				fblock_start = fblk;
			}

			if ((fblock_start + fblock_count) != fblk)
				break;

			fblock_count++;
		}

		r = ext4_blocks_set_direct(file->mp->fs.bdev, u8_buf,
					   fblock_start, fblock_count);
		if (r != EOK)
			break;

		size -= block_size * fblock_count;
		u8_buf += block_size * fblock_count;
		file->fpos += block_size * fblock_count;

		if (wcnt)
			*wcnt += block_size * fblock_count;

		fblock_start = fblk;
		fblock_count = 1;

		if (rr != EOK) {
			/*ext4_fs_append_inode_block has failed and no
			 * more blocks might be written. But node size
			 * should be updated.*/
			r = rr;
			goto out_fsize;
		}
	}

	/*Stop write back cache mode*/
	ext4_block_cache_write_back(file->mp->fs.bdev, 0);

	if (r != EOK)
		goto Finish;

	if (size) {
		uint64_t off;
		if (iblk_idx < ifile_blocks) {
			r = ext4_fs_init_inode_dblk_idx(&ref, iblk_idx, &fblk);
			if (r != EOK)
				goto Finish;
		} else {
			r = ext4_fs_append_inode_dblk(&ref, &fblk, &iblk_idx);
			if (r != EOK)
				/*Node size sholud be updated.*/
				goto out_fsize;
		}

		off = fblk * block_size;
		r = ext4_block_writebytes(file->mp->fs.bdev, off, u8_buf, size);
		if (r != EOK)
			goto Finish;

		file->fpos += size;

		if (wcnt)
			*wcnt += size;
	}

out_fsize:
	if (file->fpos > file->fsize) {
		file->fsize = file->fpos;
		ext4_inode_set_size(ref.inode, file->fsize);
		ref.dirty = true;
	}

Finish:
	r = ext4_fs_put_inode_ref(&ref);

	if (r != EOK)
		ext4_trans_abort(file->mp);
	else
		ext4_trans_stop(file->mp);

	EXT4_MP_UNLOCK(file->mp);
	return r;
}
