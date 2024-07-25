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
#include <fs/ext4_block_group.h>
#include <fs/path.h>
#include <xkernel/string.h>
#include <sync.h>
#include <debug.h>

int ext4_fread(Dirent *file, unsigned long dst, unsigned int off, unsigned int n)
{
	// 定义一些变量
	size_t rcnt = 0;
	uint32_t unalg;
	uint32_t iblock_idx;
	uint32_t iblock_last;
	uint32_t block_size;
	ext4_fsblk_t fblock;
	ext4_fsblk_t fblock_start;
	uint32_t fblock_count;
	uint8_t *u8_buf = dst;
	int r;
	struct ext4_inode_ref ref;

	// 确保文件和挂载点都存在
	ASSERT(file && file->head);

	// 如果文件是只写的，返回权限错误
	/*if (file->flags & O_WRONLY)
	    return EPERM;*/

	// 如果读取大小为0，直接返回
	if (!n)
		return 0;

	struct ext4_fs *const fs = &ext4Fs->ext4_fs;
	struct ext4_sblock *const sb = &ext4Fs->superBlock.ext4_sblock;

	// 获取 inode 引用
	r = ext4_fs_get_inode_ref(ext4Fs, file->ext4_dir_en.inode, &ref,1);
	if (r != 0) {
		return r;
	}

	// 同步文件大小
	file->file_size = ext4_inode_get_size(sb, ref.inode);

	// 获取块大小
	block_size = ext4_sb_get_block_size(sb);
	// 调整读取大小，确保不超过文件剩余大小
	n = ((uint64_t)n > (file->file_size - off)) ? ((size_t)(file->file_size - off)) : n;

	// 计算块索引和未对齐的偏移量
	iblock_idx = (uint32_t)((off) / block_size);
	iblock_last = (uint32_t)((off + n) / block_size);
	unalg = (off) % block_size;

	// 如果是小于60字节的软链接
	bool softlink;
	softlink = ext4_inode_is_type(sb, ref.inode, EXT4_INODE_MODE_SOFTLINK);
	if (softlink && file->file_size < sizeof(ref.inode->blocks) && !ext4_inode_get_blocks_count(sb, ref.inode)) {
		// 直接从块数组读取内容
		char *content = (char *)ref.inode->blocks;
		if (off < file->file_size) {
			size_t len = n;
			if (unalg + n > (uint32_t)file->file_size)
				len = (uint32_t)file->file_size - unalg;
			memcpy(dst, content + unalg, len);
			if (rcnt)
				rcnt = len;
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
			r = ext4_block_readbytes(off, u8_buf, len);
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
			rcnt += len;
		iblock_idx++;
	}
	// 处理对齐的部分
	fblock_start = 0;
	fblock_count = 0;
	while (n >= block_size) {
		while (iblock_idx < iblock_last) {
			// 获取数据块索引
			r = ext4_fs_get_inode_dblk_idx(&ref, iblock_idx, &fblock, true);
			if (r != 0)
				goto Finish;

			iblock_idx++;

			if (!fblock_start)
				fblock_start = fblock;

			if ((fblock_start + fblock_count) != fblock)
				break;
			fblock_count++;
		}
		// 直接读取多个数据块
		r = ext4_blocks_get_direct(u8_buf, block_size, fblock_start, fblock_count);
		if (r != 0)
			goto Finish;
		// 更新缓冲区指针和剩余大小
		n -= block_size * fblock_count;
		u8_buf += block_size * fblock_count;
		off += block_size * fblock_count;
		// 更新读取计数
		if (rcnt)
			rcnt += block_size * fblock_count;
		fblock_start = fblock;
		fblock_count = 1;
	}

	// 处理剩余的部分
	if (n) {
		uint64_t off;
		r = ext4_fs_get_inode_dblk_idx(&ref, iblock_idx, &fblock, true);
		if (r != 0)
		goto Finish;
		off = fblock * block_size;
		r = ext4_block_readbytes(off, u8_buf, n);
		if (r != 0)
			goto Finish;

		off += n;
		// 更新读取计数
		if (rcnt)
			rcnt += n;
	}

Finish:
    // 释放 inode 引用并解锁挂载点
    ext4_fs_put_inode_ref(&ref);
    return r;
}

int ext4_fwrite(Dirent *file, unsigned long src, unsigned int off, unsigned int n)
{
	int EOK = 0;
	size_t wcnt = 0;
	uint32_t unalg;
	uint32_t iblk_idx;
	uint32_t iblock_last;
	uint32_t ifile_blocks;
	uint32_t block_size;

	uint32_t fblock_count;
	ext4_fsblk_t fblk;
	ext4_fsblk_t fblock_start;

	struct ext4_inode_ref ref;
	const uint8_t* u8_buf = src;
	int r, rr = EOK;

	ASSERT(file && file->head);

	if (!n)
		return EOK;

	struct ext4_fs* const fs = &ext4Fs->ext4_fs;
	struct ext4_sblock* const sb = &ext4Fs->superBlock.ext4_sblock;

	r = ext4_fs_get_inode_ref(ext4Fs, file->ext4_dir_en.inode, &ref,1);
	if (r != EOK) {
		return r;
	}

	/*Sync file size*/
	file->file_size = ext4_inode_get_size(sb, ref.inode);
	block_size = ext4_sb_get_block_size(sb);

	iblock_last = (uint32_t)((off + n) / block_size);
	iblk_idx = (uint32_t)(off / block_size);
	ifile_blocks = (uint32_t)((file->file_size + block_size - 1) / block_size);

	unalg = off % block_size;

	if (unalg) {
		size_t len = n;
		uint64_t off;
		if (n > (block_size - unalg))
			len = block_size - unalg;

		r = ext4_fs_init_inode_dblk_idx(&ref, iblk_idx, &fblk);
		if (r != EOK)
			goto Finish;

		off = fblk * block_size + unalg;
		r = ext4_block_writebytes(off, u8_buf, len);
		if (r != EOK)
			goto Finish;

		u8_buf += len;
		n -= len;
		off += len;

		if (wcnt)
			wcnt += len;

		iblk_idx++;
	}

	if (r != EOK)
		goto Finish;

	fblock_start = 0;
	fblock_count = 0;
	while (n >= block_size) {
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

		r = ext4_blocks_set_direct(u8_buf,
					   block_size,fblock_start, fblock_count);
		if (r != EOK)
			break;

		n -= block_size * fblock_count;
		u8_buf += block_size * fblock_count;
		off += block_size * fblock_count;

		if (wcnt)
			wcnt += block_size * fblock_count;

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

	if (r != EOK)
		goto Finish;

	if (n) {
		uint64_t off;
		if (iblk_idx < ifile_blocks){
			
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
		r = ext4_block_writebytes(off, u8_buf, n);
		if (r != EOK)
			goto Finish;

		off += n;

		if (wcnt)
			wcnt += n;
	}

out_fsize:
	if (off > file->file_size) {
		file->file_size = off;
		ext4_inode_set_size(ref.inode, file->file_size);
		ref.dirty = true;
	}

Finish:
	r = ext4_fs_put_inode_ref(&ref);

	return r;
}

static int ext4_dirent_creat(struct Dirent *baseDir, char *path, Dirent **file, int isDir) 
{
	Dirent *dir = NULL, *f = NULL;
	int name_length;
	int r;
	longEntSet longSet;
	FileSystem *fs;
	extern FileSystem *fatFs;
	struct ext4_inode_ref ref;  // 父目录的 inode 引用

	if (baseDir) {
		fs = baseDir->file_system;
	} else {
		fs = ext4Fs;
	}
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));

	/* 记录目录深度.帮助判断中间某个目录不存在的情况 */
	unsigned int pathname_depth = path_depth_cnt((char *)path);

	/* 先检查是否将全部的路径遍历 */
	dir = search_file(path,&searched_record);
	unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
	if (pathname_depth != path_searched_depth)
	{ 
		// 说明并没有访问到全部的路径,某个中间目录是不存在的
		//printk("cannot access %s: Not a directory, subpath %s is`t exist\n",path, searched_record.searched_path);
		return -1;
	}

	/*如果已经存在该文件则不能重复建立*/
	if (dir != NULL)
	{
		if(isDir == 1 && dir->type == DIRENT_DIR)
		{
			printk("directory exists: %s\n", path);

		}else if (isDir == 0 && dir->type == DIRENT_FILE)
		{
			printk("file exists: %s\n", path);
		}
		return -1;
	}
	
	/* 获取父目录的 inode */
	ext4_fs_get_inode_ref(fs, searched_record.parent_dir->ext4_dir_en.inode, &ref,0);

	/* 分配一个inode */
	struct ext4_inode_ref child_ref;
	r = ext4_fs_alloc_inode(fs, &child_ref,isDir ? EXT4_DE_DIR : EXT4_DE_REG_FILE);
	if (r != 0)
		return r;
	
	/*初始化inode*/
	ext4_fs_inode_blocks_init(fs, &child_ref);  // 初始化 inode 块
	
	/* 创建一个目录项并将其加入目录中 */
	char *name = (strrchr(path,'/')+1);
	struct ext4_dir_en *new_en = ext4_dir_add_entry(&ref, name, sizeof(name), &child_ref);
	if (new_en == NULL)
		return -1;

	f = dirent_alloc();

	/* 填写目录项中的内容 */
	name_length = ext4_dir_en_get_name_len(&ext4Fs->superBlock.ext4_sblock, new_en);
	memcpy(&f->name, new_en->name, name_length); // 复制目录项的名称
	f->ext4_dir_en.inode = ext4_dir_en_get_inode(new_en); // 获取目录项的 i-node 号
	f->ext4_dir_en.name_len = name_length; // 设置目录项的名称长度
	f->ext4_dir_en.entry_len = ext4_dir_en_get_entry_len(new_en);// 获取目录项的长度
	f->ext4_dir_en.in.inode_type = ext4_dir_en_get_inode_type(&ext4Fs->superBlock.ext4_sblock,new_en);// 获取目录项的 i-node 类型
	f->file_size = ext4_inode_get_size(&ext4Fs->superBlock.ext4_sblock,child_ref.inode);
	f->file_system = ext4Fs;
	f->parent_dirent = searched_record.parent_dir;
	f->head = f->parent_dirent->head;
	list_init(&f->child_list);//初始化dirent项的子目录项
	f->linkcnt = 1;
	f->mode = child_ref.inode->mode;
	if (isDir == 1)
	{
		f->type = DIRENT_DIR;
	}else
	{
		f->type = DIRENT_FILE;
	}
	//将dirent加入到上级目录的子Dirent列表
	list_append(&searched_record.parent_dir->child_list,&f->dirent_tag);
	if (file) {
		*file = f;
	}
	return 0;
}

int ext4_dir_creat(Dirent *baseDir, char *path, int mode)
{
	Dirent *dir = NULL;
	int ret = ext4_dirent_creat(baseDir, path, &dir, 1);
	if (ret < 0) {
		return ret;
	} else {
		return 0;
	}
}

int ext4_file_creat(struct Dirent *baseDir, char *path, Dirent **file)
{
	return ext4_dirent_creat(baseDir, path, file, 0);
}