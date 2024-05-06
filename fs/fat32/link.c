#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <linux/thread.h>
#include <asm/errno.h>
#include <fs/cluster.h>
#include <fs/filepnt.h>
#include <sync.h>
#include <debug.h>
#include <linux/memory.h>

extern struct lock mtx_file;
static int rmfile(struct Dirent *file);

/**
 * @brief 创建链接
 * @param oldDir 旧目录的Dirent。如果oldPath使用绝对路径，可以将该参数置为NULL
 * @param oldPath 相对于旧目录的文件路径。如果为绝对路径，则可以忽略oldDir
 * @param newDir 新目录的Dirent
 * @param newPath 相对于新目录的链接路径
 */
int linkat(struct Dirent *oldDir, char *oldPath, struct Dirent *newDir, char *newPath) {
	lock_acquire(&mtx_file);

	Dirent *oldFile, *newFile;
	int ret;
	if ((ret = getFile(oldDir, oldPath, &oldFile)) < 0) {
		printk("oldFile %d not found!\n", oldPath);

		lock_release(&mtx_file);
		return ret;
	}

	char path[MAX_NAME_LEN];
	dirent_get_path(oldFile, path);
	file_close(oldFile);

	if ((ret = createFile(newDir, newPath, &newFile)) < 0) {
		printk("createFile %s failed!\n", newPath);
		lock_release(&mtx_file);
		return ret;
	}

	newFile->raw_dirent.DIR_Attr |= ATTR_LINK;
	sync_dirent_rawdata_back(newFile);

	oldFile->linkcnt += 1;
	int r = file_write(newFile, 0, (unsigned long)path, 0, strlen(path) + 1);

	file_close(newFile);
	lock_release(&mtx_file);

	if (r != strlen(path) + 1) {
		printk("linkat: write path %s to link file %s failed!\n", path, newPath);
		return -EIO; // 暂时想不出其他返回值了就返回这个吧
	} else {
		return 0;
	}
}

/**
 * @brief 需要保证传入的file->refcnt == 0
 */
int rm_unused_file(struct Dirent *file) {
	ASSERT(file->refcnt == 0);
	char linked_file_path[MAX_NAME_LEN];
	int cnt = get_entry_count_by_name(file->name);
	char data = FAT32_INVALID_ENTRY;

	// 1. 如果是链接文件，则减去其链接数
	if (file->raw_dirent.DIR_Attr & ATTR_LINK) {
		ASSERT(file->file_size < MAX_NAME_LEN);
		file_read(file, 0, (unsigned long)linked_file_path, 0, MAX_NAME_LEN);

		Dirent *linked_file;
		ASSERT(getFile(NULL, linked_file_path, &linked_file)!=0);
		linked_file->linkcnt -= 1;
		file_close(linked_file);
	}

	// 2. 断开父子关系
	ASSERT(file->parent_dirent != NULL); // 不处理根目录的情况
	// 先递归删除子Dirent（由于存在意向锁，因此这样）
	if (file->type == DIRENT_DIR) {
		struct list_elem *tmp = file->child_list.head.next;
		while (tmp!=&file->child_list.tail) {
			struct list_elem *next = tmp->next;
			Dirent *file_child = elem2entry(Dirent,dirent_tag,tmp);
			rmfile(file_child);
			tmp = next;
		}
	}

	list_remove(&file->dirent_tag);// 从父亲的子Dirent列表删除
	// 3. 释放其占用的Cluster
	file_shrink(file, 0);

	// 4. 清空目录项
	for (int i = 0; i < cnt; i++) {
		int ret = file_write(file->parent_dirent, 0, (unsigned long)&data, file->parent_dir_off - i * DIR_SIZE, 1) < 0;
		if (ret != 0)
		{
			/* code */
		}
	}
	dirent_dealloc(file); // 释放目录项
	return 0;
}

/**
 * @brief 删除文件。支持递归删除文件夹
 */
static int rmfile(struct Dirent *file) {
	//若引用计数大于则报错返回
	if (file->refcnt > 1) {
		printk("File is in use\n");
		return -1;
	}
	// 因为在rm之前也获取过一次文件的引用，所以需要放掉再做下一步操作
	file_close(file);
	return rm_unused_file(file);
}

/**
 * @brief 撤销链接。即删除(链接)文件
 */
int unlinkat(struct Dirent *dir, char *path) {
	lock_acquire(&mtx_file);

	Dirent *file;
	int ret;
	if ((ret = getFile(dir, path, &file)) < 0) {
		printk("file %s not found!\n", path);
		lock_release(&mtx_file);
		return ret;
	}

	// 拒绝删除字符设备文件
	if (file->type == DIRENT_CHARDEV) {
		printk("can't unlink chardev file %s!\n", path);
		file_close(file);
		lock_release(&mtx_file);
		return -EINVAL;
	}
	ret = rmfile(file);

	lock_release(&mtx_file);
	return ret;
}

/**
 * @brief 将srcfile中的内容写入到（空的）dst_file中
 */
static void file_transfer(Dirent *src_file, Dirent *dst_file) {
	void *buf = (void *)get_kernel_pge();

	unsigned long size = src_file->file_size;
	unsigned long r1, r2;
	for (unsigned long i = 0; i < size; i += PAGE_SIZE) {
		unsigned long cur_size = MIN(size - i, PAGE_SIZE);
		r1 = file_read(src_file, 0, (unsigned long)buf, i, cur_size);
		r2 = file_write(dst_file, 0, (unsigned long)buf, i, cur_size);
		ASSERT(r1 == cur_size);
		ASSERT(r2 == cur_size);
	}

	memset((unsigned long)buf,0,PAGE_SIZE);
}

// 为了支持mount到FATfs的文件互拷贝，采用复制-删除的模式
static int mvfile(Dirent *oldfile, Dirent *newDir, char *newPath) {
	int ret;
	if (oldfile->refcnt > 1) {
		printk("other process uses this file! refcnt = %d\n", oldfile->refcnt);
		return -EBUSY; // in use
	}

	Dirent *newfile;
	if (oldfile->type == DIRENT_FILE) {
		// 1. 在新目录中尝试新建
		ASSERT(createFile(newDir, newPath, &newfile)>=0);
		file_transfer(oldfile, newfile);

		// 2. 从原目录中删除
		return rmfile(oldfile);
	} else if (oldfile->type == DIRENT_DIR) {
		// 1. 创建同名的目录
		ASSERT(makeDirAt(newDir, newPath, 0)>=0);

		// 2. 遍历oldFile目录下的文件，递归，如果中途有错误，就立刻返回
		// 名称newPath由kmalloc分配，记得释放（之所以不在栈上是为了防止溢出）
		Dirent *child;
		struct list_elem *tmp = oldfile->child_list.head.next;
		while (tmp!=&oldfile->child_list.tail)
		{
			child = elem2entry(Dirent,dirent_tag,tmp);
			char new_child_path[MAX_NAME_LEN] ;
			/*char *new_child_path = kmalloc(MAX_NAME_LEN);*/
			strcpy(new_child_path, newPath);
			strcat(new_child_path, "/");
			strcat(new_child_path, child->name);
			// 递归
			ret = mvfile(child, newDir, new_child_path);
			//kfree(new_child_path);

			if (ret < 0) {
				printk("mvfile: mv child %s failed! (partially move)\n", child->name);
				return ret;
			}
			tmp = tmp->next;
		}
		// 3. 删除旧的目录
		return rmfile(oldfile);
	} else {
		printk("unknown dirent type: %d\n", oldfile->type);
	}
	return 0;
}

// TODO: 处理flags
int renameat2(Dirent *oldDir, char *oldPath, Dirent *newDir, char *newPath, u32 flags) {
	lock_acquire(&mtx_file);

	// 1. 前置的检查，获取oldFile
	Dirent *oldFile, *newFile;
	int ret;
	if ((ret = getFile(oldDir, oldPath, &oldFile)) < 0) {
		printk("renameat2: oldFile %s not found!\n", oldPath);
		lock_release(&mtx_file);
		return ret;
	}

	// 不能移动字符设备文件
	if (oldFile->type == DIRENT_CHARDEV) {
		printk("renameat2: can't rename chardev file %s!\n", oldPath);
		file_close(oldFile);
		lock_release(&mtx_file);
		return -EINVAL;
	}

	if ((ret = getFile(newDir, newPath, &newFile)) == 0) {
		printk("renameat2: newFile %s exists!\n", newPath);
		file_close(newFile);
		lock_release(&mtx_file);
		return -EEXIST;
	}

	ret = mvfile(oldFile, newDir, newPath);
	lock_release(&mtx_file);
	return ret;
}
