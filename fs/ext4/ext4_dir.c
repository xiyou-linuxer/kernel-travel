#include <fs/ext4.h>
#include <fs/fs.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_sb.h>
#include <fs/mount.h>
#include <asm-generic/errno-base.h>

/**
 * @brief 获取目录条目长度
 * @param de Directory 项
 * @return 返回值为entry的长度
 */
static inline uint16_t ext4_dir_en_get_entry_len(struct ext4_dir_en *de)
{
	return to_le16(de->entry_len);
}

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
        return EIO;

    // 确保目录项的核心不会溢出块的边界
    if (off_in_block > block_size - 8)
        return EIO;

    struct ext4_dir_en *en;
    // 将 en 指针设置为当前块数据的偏移位置
    en = (void *)(it->curr_blk.buf->data + off_in_block);

    // 确保整个目录项不会溢出块的边界
    uint16_t length = ext4_dir_en_get_entry_len(en);
    if (off_in_block + length > block_size)
        return EIO;

    // 确保目录项中的名称长度不过大
    if (ext4_dir_en_get_name_len(sb, en) > length - 8)
        return EIO;

    // 一切检查通过后，设置当前目录项指针
    it->curr = en;
    return 0;
}

static int ext4_dir_iterator_seek(struct ext4_dir_iter *it, uint64_t pos)
{
	struct ext4_sblock *sb = &it->pdirent->file_system->superBlock; // 超级块指针
	struct ext4_inode *inode = it->pdirent->ext4_dir_en.inode;//获取迭代器对应目录对应的inode
	uint64_t size = ext4_inode_get_size(sb, inode); // 获取 i-node 的大小
	int r; 

	/* 迭代器在定位到所需位置之前是无效的 */
	it->curr = NULL;

	/* 检查是否已到达末尾 */
	if (pos >= size) {
		// 如果位置超出了 i-node 的大小，则处理结束
		if (it->curr_blk.lb_id) {
			// 如果当前块有效，则设置块并清空当前块信息
			it->curr_blk.lb_id = 0;
			it->curr_blk.buf = NULL;
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
			it->curr_blk.buf = NULL;
			it->curr_blk.lb_id = 0;
			if (r != 0)
				return r;
		}

		uint64_t next_blk;
		r = ext4_fs_get_inode_dblk_idx(it->inode_ref, next_blk_idx, &next_blk, false); // 获取下一个块地址
		if (r != 0)
			return r;
		r = bufRead(1,next_blk,1);// 获取块数据
		if (r != 0) {
			it->curr_blk.lb_id = 0;
			return r;
		}
	}

	it->curr_off = pos; // 更新当前位置为指定位置
	return ext4_dir_iterator_set(it, block_size); // 设置迭代器到指定位置
}

/*初始化目录迭代器*/
int ext4_dir_iterator_init(struct ext4_dir_iter *it,
			   struct Dirent *pdirent, uint64_t pos)
{
	it->pdirent = pdirent;
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
const Dirent *ext4_dir_entry_next(struct ext4_dir *dir)
{
#define EXT4_DIR_ENTRY_OFFSET_TERM (uint64_t)(-1)

	int r;
	uint16_t name_length;
	Dirent *de = 0; // 指向目录项的指针
	struct ext4_dir_iter it; // 目录迭代器
	struct ext4_inode_ref dir_inode; // 目录 i-node 的引用
	if (dir->next_off == EXT4_DIR_ENTRY_OFFSET_TERM) { // 如果已经遍历到目录尾部
		return 0; // 返回空指针
	}
	// 获取目录 i-node 的引用
	r = ext4_fs_get_inode_ref(&dir->pdirent->head, dir->pdirent->ext4_dir_en.inode, &dir_inode);
	if (r != 0) {
		goto Finish; // 发生错误，跳转到结束处理
	}

	// 初始化目录迭代器，从指定偏移量开始
	r = ext4_dir_iterator_init(&it, &dir->pdirent, dir->next_off);

	memset(&dir->de.name, 0, sizeof(dir->de.name)); // 清空目录项的名称字段
	name_length = ext4_dir_en_get_name_len(&dir->pdirent->head->mnt_rootdir->file_system->superBlock.ext4_sblock, it.curr); // 获取目录项名称长度
	memcpy(&dir->de.name, it.curr->name, name_length); // 复制目录项的名称

	// 复制目录项的信息到目录项结构
	dir->de.ext4_dir_en.inode = ext4_dir_en_get_inode(it.curr); // 获取目录项的 i-node 号
	dir->de.ext4_dir_en.entry_len = ext4_dir_en_get_entry_len(it.curr); // 获取目录项的长度
	dir->de.ext4_dir_en.name_len = name_length; // 设置目录项的名称长度
	dir->de.ext4_dir_en.in.inode_type = ext4_dir_en_get_inode_type(&dir->pdirent->head->mnt_rootdir->file_system->superBlock.ext4_sblock, it.curr); // 获取目录项的 i-node 类型

	de = &dir->de; // 设置目录项指针为当前目录项
	de->file_system = ext4Fs;
	de->type = EXT4_IS_DIR(dir_inode.inode->mode) ? DIRENT_DIR : DIRENT_FILE;
	de->parent_dirent = dir->pdirent;
	list_init(&de->child_list);
	de->linkcnt = 1;
	de->mode = dir_inode.inode->mode;
	ext4_dir_iterator_next(&it); // 移动到下一个目录项

	// 更新下一个目录项的偏移量，如果没有下一个目录项，则设置为终止偏移量
	dir->next_off = it.curr ? it.curr_off : EXT4_DIR_ENTRY_OFFSET_TERM;
	de->parent_dir_off = dir->next_off;
	ext4_dir_iterator_fini(&it); // 结束目录迭代器的使用
	ext4_fs_put_inode_ref(&dir_inode); // 释放目录 i-node 的引用
Finish:
	return de; // 返回下一个目录项的指针，如果没有更多目录项可用，则返回NULL
}
