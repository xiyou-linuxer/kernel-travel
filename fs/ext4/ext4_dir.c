#include <fs/ext4.h>
#include <fs/fs.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_sb.h>
#include <fs/mount.h>
#include <fs/dirent.h>
#include <fs/ext4_crc32.h>
#include <asm-generic/errno-base.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <debug.h>
/** 
 * @brief 在返回迭代器之前进行一些检查。
 * @param it 需要检查的迭代器
 * @param block_size 数据块的大小
 * @return 错误代码
 */
static int ext4_dir_iterator_set(struct ext4_dir_iter *it, uint32_t block_size)
{
	// 计算当前偏移在块中的位置
	uint32_t off_in_block = it->curr_off % block_size;
	struct ext4_sblock *sb = &it->inode_ref->fs->superBlock.ext4_sblock;

	// 初始化当前条目指针为 NULL
	it->curr = NULL;
	
	// 确保偏移在块内的对齐是正确的（必须是 4 的倍数）
	if ((off_in_block % 4) != 0)
	{
		return EIO;
	}
	// 确保目录项的核心不会溢出块的边界
	if (off_in_block > block_size - 8)
	{
		return EIO;
	}
	struct ext4_dir_en *en;
	// 将 en 指针设置为当前块数据的偏移位置
	en = (void *)(it->curr_blk.buf->data->data + off_in_block);
	// 确保整个目录项不会溢出块的边界
	uint16_t length = ext4_dir_en_get_entry_len(en);
	if (off_in_block + length > block_size)
	{
		return EIO;
	}

	int name_len = ext4_dir_en_get_name_len(sb, en);
	// 确保目录项中的名称长度不过大
	if (name_len > length - 8)
	{
		return EIO;
	}

	// 一切检查通过后，设置当前目录项指针
	it->curr = en;
	return 0;
}

static int ext4_dir_iterator_seek(struct ext4_dir_iter *it, uint64_t pos)
{
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock; // 超级块指针
	struct ext4_inode *inode = it->inode_ref->inode;//获取迭代器对应目录对应的inode
	
	uint64_t size = ext4_inode_get_size(sb, inode); // 获取 i-node 的大小
	int r; 
	/* 迭代器在定位到所需位置之前是无效的 */
	it->curr = NULL;
	//printk("size:%d\n",size);
	/* 检查是否已到达末尾 */
	if (pos >= size) {
		//printk("pos :%d size:%d \n",pos,size);
		// 如果位置超出了 i-node 的大小，则处理结束
		if (it->curr_blk.lb_id) {
			// 如果当前块有效，则设置块并清空当前块信息
			bufRelease(it->curr_blk.buf);
			it->curr_blk.lb_id = 0;
			it->curr_blk.buf->data = 0;
			if (r != 0)
				return r;
		}
		it->curr_off = pos; // 更新当前位置为指定位置
		return 0; // 返回操作成功
	}

	/* 计算下一个块地址 */
	uint32_t block_size = ext4_sb_get_block_size(sb); // 获取块大小
	uint64_t current_blk_idx = it->curr_off / block_size; // 计算当前块索引
	uint32_t next_blk_idx = (uint32_t)(pos / block_size); // 计算下一个块索引

	/*
	 * 如果当前块无效或者需要跨块移动，
	 * 则需要获取另一个数据块
	 */
	if ((it->curr_blk.lb_id == 0) || (current_blk_idx != next_blk_idx)) {
		// 如果当前块无效或者需要跨块移动，则获取下一个块
		if (it->curr_blk.lb_id) {
			//如果当前块无效则将块缓冲区置空
			bufRelease(it->curr_blk.buf);
			it->curr_blk.lb_id = 0;
			it->curr_blk.buf->data = 0;
			if (r != 0)
				return r;
		}
		uint64_t next_blk;
		r = ext4_fs_get_inode_dblk_idx(it->inode_ref, next_blk_idx, &next_blk, false); // 获取下一个块地址
		//printk("next_blk:%d\n",next_blk);
		if (r != 0)
			return r;
		it->curr_blk.buf = bufRead(1,EXT4_LBA2PBA(next_blk),1);// 获取块数据
		it->curr_blk.lb_id = next_blk;
		if (it->curr_blk.buf == NULL) {
			it->curr_blk.lb_id = 0;
			return r;
		}
	}
	it->curr_off = pos; // 更新当前位置为指定位置
	
	return ext4_dir_iterator_set(it, block_size); // 设置迭代器到指定位置
}

/*初始化目录迭代器*/
int ext4_dir_iterator_init(struct ext4_dir_iter *it, struct ext4_inode_ref *inode_ref, uint64_t pos)
{
	it->inode_ref = inode_ref;
	it->curr = 0;
	it->curr_off = 0;
	it->curr_blk.lb_id = 0;
	return ext4_dir_iterator_seek(it, pos);
}


int ext4_dir_iterator_fini(struct ext4_dir_iter *it)
{
	it->curr = 0;

	if (it->curr_blk.lb_id)
		it->curr_blk.buf == NULL;

	return 0;
}

int ext4_dir_iterator_next(struct ext4_dir_iter *it)
{
	int r = 0;
	uint16_t skip;

	while (r == 0) {
		skip = ext4_dir_en_get_entry_len(it->curr);
		r = ext4_dir_iterator_seek(it, it->curr_off + skip);

		if (!it->curr)
			break;
		/*Skip NULL referenced entry*/
		if (ext4_dir_en_get_inode(it->curr) != 0)
			break;
	}

	return r;
}

/**
 * @brief 获取目录中的下一个目录项。
 * 
 * 该函数用于迭代目录中的所有目录项，并返回下一个目录项的指针。
 * 如果没有更多的目录项可用，返回NULL。
 * 
 * @param dir 指向 ext4_dir 结构的指针，表示目录对象。
 * @return const Dirent * 指向下一个目录项的指针，如果没有更多目录项可用，则返回NULL。
 */
struct Dirent *ext4_dir_entry_next(struct ext4_dir *dir)
{
	int r;
	uint16_t name_length;
	Dirent *de = 0; // 指向目录项的指针
	struct ext4_dir_iter it; // 目录迭代器
	struct ext4_inode_ref dir_inode; // 目录 i-node 的引用
	if (dir->next_off == EXT4_DIR_ENTRY_OFFSET_TERM) { // 如果已经遍历到目录尾部
		return NULL; // 返回空指针
	}
	// 获取目录 i-node 的引用
	r = ext4_fs_get_inode_ref(ext4Fs, dir->pdirent->ext4_dir_en.inode, &dir_inode,1);
	if (r != 0) {
		goto Finish; // 发生错误，跳转到结束处理
	}

	// 初始化目录迭代器，从指定偏移量开始
	r = ext4_dir_iterator_init(&it, &dir_inode, dir->next_off);
	if (r != 0) {
		ext4_fs_put_inode_ref(&dir_inode); // 释放目录 i-node 的引用
		goto Finish; // 发生错误，跳转到结束处理
	}

	dir->de = dirent_alloc();

	memset(&dir->de->name, 0, sizeof(dir->de->name)); // 清空目录项的名称字段
	name_length = ext4_dir_en_get_name_len(&ext4Fs->superBlock.ext4_sblock, it.curr); // 获取目录项名称长度
	memcpy(&dir->de->name, it.curr->name, name_length); // 复制目录项的名称
	// 复制目录项的信息到目录项结构
	dir->de->ext4_dir_en.inode = ext4_dir_en_get_inode(it.curr); // 获取目录项的 i-node 号
	dir->de->ext4_dir_en.entry_len = ext4_dir_en_get_entry_len(it.curr); // 获取目录项的长度
	dir->de->ext4_dir_en.name_len = name_length; // 设置目录项的名称长度
	dir->de->ext4_dir_en.in.inode_type = ext4_dir_en_get_inode_type(&ext4Fs->superBlock.ext4_sblock, it.curr); // 获取目录项的 i-node 类型
	de = dir->de; // 设置目录项指针为当前目录项
	de->file_size = ext4_inode_get_size(&ext4Fs->superBlock.ext4_sblock,dir_inode.inode);
	de->file_system = ext4Fs;
	de->parent_dirent = dir->pdirent;
	list_init(&de->child_list);//初始化dirent项的子目录项
	de->linkcnt = 1;
	de->mode = dir_inode.inode->mode;
	ext4_dir_iterator_next(&it); // 移动到下一个目录项
	if (de->ext4_dir_en.in.inode_type == EXT4_INODE_MODE_SOFTLINK);
	{
		de->type = DIRENT_SOFTLINK;
	}
	// 更新下一个目录项的偏移量，如果没有下一个目录项，则设置为终止偏移量
	dir->next_off = it.curr ? it.curr_off : EXT4_DIR_ENTRY_OFFSET_TERM;
	de->parent_dir_off = dir->next_off;
	ext4_dir_iterator_fini(&it); // 结束目录迭代器的使用
	ext4_fs_put_inode_ref(&dir_inode); // 释放目录 i-node 的引用
Finish:
	return de; // 返回下一个目录项的指针，如果没有更多目录项可用，则返回NULL
}

static struct ext4_dir_entry_tail *ext4_dir_get_tail(struct ext4_inode_ref *inode_ref, struct ext4_dir_en *de)
{
	struct ext4_dir_entry_tail *t;
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;

	t = EXT4_DIRENT_TAIL(de, ext4_sb_get_block_size(sb));

	if (t->reserved_zero1 || t->reserved_zero2)
		return NULL;
	if (to_le16(t->rec_len) != sizeof(struct ext4_dir_entry_tail))
		return NULL;
	if (t->reserved_ft != EXT4_DIRENTRY_DIR_CSUM)
		return NULL;

	return t;
}

static uint32_t ext4_dir_csum(struct ext4_inode_ref *inode_ref, struct ext4_dir_en *dirent, int size)
{
	uint32_t csum;
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
	uint32_t ino_index = to_le32(inode_ref->index);
	uint32_t ino_gen = to_le32(ext4_inode_get_generation(inode_ref->inode));

	/* First calculate crc32 checksum against fs uuid */
	csum = ext4_crc32c(EXT4_CRC32_INIT, sb->uuid, sizeof(sb->uuid));
	/* Then calculate crc32 checksum against inode number
	 * and inode generation */
	csum = ext4_crc32c(csum, &ino_index, sizeof(ino_index));
	csum = ext4_crc32c(csum, &ino_gen, sizeof(ino_gen));
	/* Finally calculate crc32 checksum against directory entries */
	csum = ext4_crc32c(csum, dirent, size);
	return csum;
}

void ext4_dir_set_csum(struct ext4_inode_ref *inode_ref,
			   struct ext4_dir_en *dirent)
{
	struct ext4_dir_entry_tail *t;
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;

	/* Compute the checksum only if the filesystem supports it */
	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM)) {
		t = ext4_dir_get_tail(inode_ref, dirent);
		if (!t) {
			/* There is no space to hold the checksum */
			return;
		}

		int __attribute__ ((__unused__)) diff = (char *)t - (char *)dirent;
		uint32_t csum = ext4_dir_csum(inode_ref, dirent, diff);
		t->checksum = to_le32(csum);
	}
}

void ext4_dir_init_entry_tail(struct ext4_dir_entry_tail *t)
{
	memset(t, 0, sizeof(struct ext4_dir_entry_tail));
	t->rec_len = to_le16(sizeof(struct ext4_dir_entry_tail));
	t->reserved_ft = EXT4_DIRENTRY_DIR_CSUM;
}

void ext4_dir_write_entry(struct ext4_sblock *sb, struct ext4_dir_en *en,
			  uint16_t entry_len, struct ext4_inode_ref *child,
			  const char *name, size_t name_len)
{
	/* Check maximum entry length */
	ASSERT(entry_len <= ext4_sb_get_block_size(sb));

	/* 设置目录项的属性 */
	switch (ext4_inode_type(sb, child->inode)) {
	case EXT4_INODE_MODE_DIRECTORY:
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_DIR);
		break;
	case EXT4_INODE_MODE_FILE:
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_REG_FILE);
		break;
	case EXT4_INODE_MODE_SOFTLINK:
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_SYMLINK);
		break;
	case EXT4_INODE_MODE_CHARDEV:
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_CHRDEV);
		break;
	case EXT4_INODE_MODE_BLOCKDEV:
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_BLKDEV);
		break;
	case EXT4_INODE_MODE_FIFO:
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_FIFO);
		break;
	case EXT4_INODE_MODE_SOCKET:
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_SOCK);
		break;
	default:
		/* FIXME: unsupported filetype */
		ext4_dir_en_set_inode_type(sb, en, EXT4_DE_UNKNOWN);
	}

	/* 设置 inode 号等 */
	ext4_dir_en_set_inode(en, child->index);
	ext4_dir_en_set_entry_len(en, entry_len);
	ext4_dir_en_set_name_len(sb, en, (uint16_t)name_len);

	/* 写入目录名 */
	memcpy(en->name, name, name_len);
}


struct ext4_dir_en * ext4_dir_try_insert_entry(struct ext4_sblock *sb,struct ext4_inode_ref *inode_ref, int fblock, struct ext4_inode_ref *child, const char *name, uint32_t name_len)
{
	struct ext4_block dst_blk;
	// 计算所需的条目长度，并将其对齐到 4 字节
	uint32_t block_size = ext4_sb_get_block_size(sb);
	uint16_t required_len = sizeof(struct ext4_fake_dir_entry) + name_len;

	if ((required_len % 4) != 0)
		required_len += 4 - (required_len % 4);

	// 初始化指针，stop 指针指向块的上限
	
	char lp = block_size/BUF_SIZE;
	for (int i = 0; i < lp; i++)
	{
		dst_blk.buf = bufRead(1,(lp*fblock)+i,1);
		struct ext4_dir_en *start = (void *)dst_blk.buf->data->data;
		struct ext4_dir_en *stop = (void *)(dst_blk.buf->data->data + BUF_SIZE);
		/* 遍历块，检查无效条目或具有足够空间的新条目 */
		while (start < stop) {
			uint32_t inode = ext4_dir_en_get_inode(start);
			uint16_t rec_len = ext4_dir_en_get_entry_len(start);
			uint8_t itype = ext4_dir_en_get_inode_type(sb, start);

			// 如果条目无效且空间足够大，使用该条目
			if ((inode == 0) && (itype != EXT4_DIRENTRY_DIR_CSUM) && (rec_len >= required_len)) {
				ext4_dir_write_entry(sb, start, rec_len, child, name, name_len);
				ext4_dir_set_csum(inode_ref, (void *)dst_blk.buf->data->data);
				bufWrite(dst_blk.buf);
				return start;//返回当前条目
			}

			// 有效条目，尝试拆分
			if (inode != 0) {
				uint16_t used_len;
				used_len = ext4_dir_en_get_name_len(sb, start);

				uint16_t sz;
				sz = sizeof(struct ext4_fake_dir_entry) + used_len;

				if ((used_len % 4) != 0)
					sz += 4 - (used_len % 4);

				uint16_t free_space = rec_len - sz;

			// 如果有足够的空闲空间用于新条目
				if (free_space >= required_len) {
				// 切割当前条目的尾部
				/* Cut tail of current entry */
					struct ext4_dir_en * new_entry;
					new_entry = (void *)((uint8_t *)start + sz);
					ext4_dir_en_set_entry_len(start, sz);
					ext4_dir_write_entry(sb, new_entry, free_space,
						     child, name, name_len);
					printk("num:%d\n",(lp*fblock)+i);
					ext4_dir_set_csum(inode_ref,
						  (void *)dst_blk.buf->data->data);
					bufWrite(dst_blk.buf);
					return new_entry;
				}
			}	

			// 跳转到下一个条目
			start = (void *)((uint8_t *)start + rec_len);
		}
		bufRelease(dst_blk.buf);
	}
	// 未找到适合新条目的空闲空间
	return NULL;
}

/*创建一个新的ext4_dir_en，并将其添加到目录文件中*/
struct ext4_dir_en * ext4_dir_add_entry(struct ext4_inode_ref *parent, const char *name, uint32_t name_len, struct ext4_inode_ref *child)
{
	int r;
	struct ext4_fs *fs = &ext4Fs->ext4_fs;
	struct ext4_sblock *sb = &ext4Fs->superBlock.ext4_sblock;
	//int EOK = 0;
	// 线性算法
	/* Linear algorithm */
	uint32_t iblock = 0;
	ext4_fsblk_t fblock = 0;
	uint32_t block_size = ext4_sb_get_block_size(sb);
	uint64_t inode_size = ext4_inode_get_size(sb, parent->inode);
	uint32_t total_blocks = (uint32_t)(inode_size / block_size);

	// 查找有空间的新目录项的块并尝试添加
	/* Find block, where is space for new entry and try to add */
	bool success = false;
	for (iblock = 0; iblock < total_blocks; ++iblock) {
		r = ext4_fs_get_inode_dblk_idx(parent, iblock, &fblock, false);
		if (r != EOK)
			return r;

		// 如果添加成功，函数可以结束
		struct ext4_dir_en * new_en = ext4_dir_try_insert_entry(sb, parent, fblock, child,name, name_len);
		if (new_en!=NULL)
		{
			return new_en;
		}
	}
	// 没有找到空闲块 - 需要分配下一个数据块
	/* No free block found - needed to allocate next data block */
	iblock = 0;
	fblock = 0;
	r = ext4_fs_append_inode_dblk(parent, &fblock, &iblock);
	if (r != EOK)
		return r;

	// 加载新块
	/* Load new block */
	struct ext4_block b;
	r = bufRead(1,EXT4_LBA2PBA(fblock),0);
	
	// 将块填充为零
	memset(b.buf->data->data, 0, block_size);
	struct ext4_dir_en *blk_en = (void *)b.buf->data->data;
	
	// 保存新块
	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM)) {
		uint16_t el = block_size - sizeof(struct ext4_dir_entry_tail);
		ext4_dir_write_entry(sb, blk_en, el, child, name, name_len);
		ext4_dir_init_entry_tail(EXT4_DIRENT_TAIL(b.buf->data->data, block_size));
	} else {
		ext4_dir_write_entry(sb, blk_en, block_size, child, name,
				name_len);
	}

	ext4_dir_set_csum(parent, (void *)b.buf->data->data);
	bufWrite(b.buf);
	bufRelease(b.buf);
	
	return blk_en;
}
