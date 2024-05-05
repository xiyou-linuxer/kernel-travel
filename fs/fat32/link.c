#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <proc/cpu.h>
#include <linux/thread.h>
#include <proc/proc.h>
#include <asm/errno.h>
#include <fs/cluster.h>
#include <fs/filepnt.h>

extern mutex_t mtx_file;
static int rmfile(struct Dirent *file);

/**
 * @brief 创建链接
 * @param oldDir 旧目录的Dirent。如果oldPath使用绝对路径，可以将该参数置为NULL
 * @param oldPath 相对于旧目录的文件路径。如果为绝对路径，则可以忽略oldDir
 * @param newDir 新目录的Dirent
 * @param newPath 相对于新目录的链接路径
 */
int linkat(struct Dirent *oldDir, char *oldPath, struct Dirent *newDir, char *newPath) {
	mtx_lock_sleep(&mtx_file);

	Dirent *oldFile, *newFile;
	int ret;
	if ((ret = getFile(oldDir, oldPath, &oldFile)) < 0) {
		warn("oldFile %d not found!\n", oldPath);

		mtx_unlock_sleep(&mtx_file);
		return ret;
	}

	char path[MAX_NAME_LEN];
	dirent_get_path(oldFile, path);
	file_close(oldFile);

	if ((ret = createFile(newDir, newPath, &newFile)) < 0) {
		warn("createFile %s failed!\n", newPath);
		mtx_unlock_sleep(&mtx_file);
		return ret;
	}

	newFile->raw_dirent.DIR_Attr |= ATTR_LINK;
	sync_dirent_rawdata_back(newFile);

	oldFile->linkcnt += 1;
	int r = file_write(newFile, 0, (u64)path, 0, strlen(path) + 1);

	file_close(newFile);
	mtx_unlock_sleep(&mtx_file);

	if (r != strlen(path) + 1) {
		warn("linkat: write path %s to link file %s failed!\n", path, newPath);
		return -EIO; // 暂时想不出其他返回值了就返回这个吧
	} else {
		return 0;
	}
}

// /**
//  * @brief 递归地从文件的尾部释放其cluster
//  * @note 有栈溢出风险
//  */
// static void recur_free_clus(FileSystem *fs, int clus, int prev_clus) {
// 	int next_clus = fatRead(fs, clus);
// 	if (!FAT32_NOT_END_CLUSTER(next_clus)) {
// 		// clus is end cluster
// 		clusterFree(fs, clus, prev_clus);
// 	} else {
// 		recur_free_clus(fs, next_clus, clus);
// 		clusterFree(fs, clus, prev_clus);
// 	}
// }

/**
 * @brief 需要保证传入的file->refcnt == 0
 */
int rm_unused_file(struct Dirent *file) {
	assert(file->refcnt == 0);
	char linked_file_path[MAX_NAME_LEN];
	int cnt = get_entry_count_by_name(file->name);
	char data = FAT32_INVALID_ENTRY;

	// 1. 如果是链接文件，则减去其链接数
	if (file->raw_dirent.DIR_Attr & ATTR_LINK) {
		assert(file->file_size < MAX_NAME_LEN);
		file_read(file, 0, (u64)linked_file_path, 0, MAX_NAME_LEN);

		Dirent *linked_file;
		panic_on(getFile(NULL, linked_file_path, &linked_file));
		linked_file->linkcnt -= 1;
		file_close(linked_file);
	}

	// 2. 断开父子关系
	assert(file->parent_dirent != NULL); // 不处理根目录的情况
	// 先递归删除子Dirent（由于存在意向锁，因此这样）
	if (file->type == DIRENT_DIR) {
		Dirent *tmp;
		LIST_FOREACH (tmp, &file->child_list, dirent_link) {
			rmfile(tmp);
		}
	}
	LIST_REMOVE(file, dirent_link); // 从父亲的子Dirent列表删除

	// 3. 释放其占用的Cluster
	file_shrink(file, 0);

	// 4. 清空目录项
	for (int i = 0; i < cnt; i++) {
		panic_on(file_write(file->parent_dirent, 0, (u64)&data,
				    file->parent_dir_off - i * DIR_SIZE, 1) < 0);
	}

	dirent_dealloc(file); // 释放目录项
	return 0;
}

/**
 * @brief 删除文件。支持递归删除文件夹
 */
static int rmfile(struct Dirent *file) {
	if (file->refcnt > 1) {
		// 检查是否都是当前进程(TODO: 目前为线程)持有此文件，如果是，可以直接删除
		int hold_by_cur = 1;
		for (int i = 0; i < file->holder_cnt; i++) {
			if (file->holders[i].proc_index != get_proc_index(cpu_this()->cpu_running->td_proc)) {
				hold_by_cur = 0;
				break;
			}
		}

		if (!hold_by_cur) {
			warn("other process uses file %s! refcnt = %d\n", file->name, file->refcnt);

#ifdef REFCNT_DEBUG
			for (int i = 0; i < file->holder_cnt; i++) {
				warn("holder: %d, cnt = %d\n", file->holders[i].proc_index, file->holders[i].cnt);
			}
#endif

			return -EBUSY; // in use
		} else {
			// 自己持有的
			warn("file %s is hold by current process(refcnt = %d), can't remove, will remove on close, "
			     "continue!\n",
			     file->name, file->refcnt);
			// 因为在rm之前也获取过一次文件的引用
			file_close(file);
			file->is_rm = 1;
			return 0;
			/**
			 * 此实现是为了应对ftello_unflushed_append的unlink失败问题
			 * TODO: 后续实现建议：
			 * 在Dirent上置一个位is_rm，表示在refcnt归0时是否可以删除。如果可以删除，那么在
			 * file_close()之后检测到 (refcnt == 0 && is_rm) 就将其删除
			 */
		}
	}

	// 因为在rm之前也获取过一次文件的引用，所以需要放掉再做下一步操作
	file_close(file);
	return rm_unused_file(file);
}

/**
 * @brief 撤销链接。即删除(链接)文件
 */
int unlinkat(struct Dirent *dir, char *path) {
	mtx_lock_sleep(&mtx_file);

	Dirent *file;
	int ret;
	if ((ret = getFile(dir, path, &file)) < 0) {
		warn("file %s not found!\n", path);
		mtx_unlock_sleep(&mtx_file);
		return ret;
	}

	// 拒绝删除字符设备文件
	if (file->type == DIRENT_CHARDEV) {
		warn("can't unlink chardev file %s!\n", path);
		file_close(file);
		mtx_unlock_sleep(&mtx_file);
		return -EINVAL;
	}
	ret = rmfile(file);

	mtx_unlock_sleep(&mtx_file);
	return ret;
}

/**
 * @brief 将srcfile中的内容写入到（空的）dst_file中
 */
static void file_transfer(Dirent *src_file, Dirent *dst_file) {
	void *buf = (void *)kvmAlloc();

	u64 size = src_file->file_size;
	u64 r1, r2;
	for (u64 i = 0; i < size; i += PAGE_SIZE) {
		u64 cur_size = MIN(size - i, PAGE_SIZE);
		r1 = file_read(src_file, 0, (u64)buf, i, cur_size);
		r2 = file_write(dst_file, 0, (u64)buf, i, cur_size);
		assert(r1 == cur_size);
		assert(r2 == cur_size);
	}

	kvmFree((u64)buf);
}

// 为了支持mount到FATfs的文件互拷贝，采用复制-删除的模式
static int mvfile(Dirent *oldfile, Dirent *newDir, char *newPath) {
	int ret;
	if (oldfile->refcnt > 1) {
		warn("other process uses this file! refcnt = %d\n", oldfile->refcnt);

#ifdef REFCNT_DEBUG
		for (int i = 0; i < oldfile->holder_cnt; i++) {
			warn("holder: %d, cnt: %d\n", oldfile->holders[i].proc_index, oldfile->holders[i].cnt);
		}
#endif
		return -EBUSY; // in use
	}

	Dirent *newfile;
	if (oldfile->type == DIRENT_FILE) {
		// 1. 在新目录中尝试新建
		unwrap(createFile(newDir, newPath, &newfile));
		file_transfer(oldfile, newfile);

		// 2. 从原目录中删除
		return rmfile(oldfile);
	} else if (oldfile->type == DIRENT_DIR) {
		// 1. 创建同名的目录
		unwrap(makeDirAt(newDir, newPath, 0));

		// 2. 遍历oldFile目录下的文件，递归，如果中途有错误，就立刻返回
		// 名称newPath由kmalloc分配，记得释放（之所以不在栈上是为了防止溢出）
		Dirent *child;
		LIST_FOREACH (child, &oldfile->child_list, dirent_link) {
			char *new_child_path = kmalloc(MAX_NAME_LEN);
			strncpy(new_child_path, newPath, MAX_NAME_LEN);
			strcat(new_child_path, "/");
			strcat(new_child_path, child->name);

			// 递归
			ret = mvfile(child, newDir, new_child_path);
			kfree(new_child_path);

			if (ret < 0) {
				warn("mvfile: mv child %s failed! (partially move)\n", child->name);
				return ret;
			}
		}

		// 3. 删除旧的目录
		return rmfile(oldfile);
	} else {
		panic("unknown dirent type: %d\n", oldfile->type);
	}
	return 0;
}

// TODO: 处理flags
int renameat2(Dirent *oldDir, char *oldPath, Dirent *newDir, char *newPath, u32 flags) {
	mtx_lock_sleep(&mtx_file);

	// 1. 前置的检查，获取oldFile
	Dirent *oldFile, *newFile;
	int ret;
	if ((ret = getFile(oldDir, oldPath, &oldFile)) < 0) {
		warn("renameat2: oldFile %s not found!\n", oldPath);
		mtx_unlock_sleep(&mtx_file);
		return ret;
	}

	// 不能移动字符设备文件
	if (oldFile->type == DIRENT_CHARDEV) {
		warn("renameat2: can't rename chardev file %s!\n", oldPath);
		file_close(oldFile);
		mtx_unlock_sleep(&mtx_file);
		return -EINVAL;
	}

	if ((ret = getFile(newDir, newPath, &newFile)) == 0) {
		warn("renameat2: newFile %s exists!\n", newPath);
		file_close(newFile);
		mtx_unlock_sleep(&mtx_file);
		return -EEXIST;
	}

	ret = mvfile(oldFile, newDir, newPath);
	mtx_unlock_sleep(&mtx_file);
	return ret;
}
