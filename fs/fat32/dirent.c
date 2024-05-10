#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/fat32.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <sync.h>
#include <linux/thread.h>
#include <fs/filepnt.h>
#include <debug.h>
#include <asm/errno.h>
#include <linux/list.h>
unsigned long used_dirents = 0;

/**
 * 本文件用于维护Dirent的分配以及维护和修改Dirent的树状结构
 */

extern struct lock mtx_file;

// 待分配的dirent
Dirent dirents[4096];
struct list dirent_free_list;

#define PEEK sizeof(dirents)

// 管理dirent分配和释放的互斥锁
struct lock mtx_dirent;

void dirent_init(void) 
{
	lock_init(&mtx_dirent);
	list_init(&dirent_free_list);
	for (int i = 0; i < 4096; i++) {
		list_append(&dirent_free_list,&dirents[i].dirent_tag);
	}
	printk("dirent Init Finished!\n");
}

// Dirent的分配和释放属于对Dirent整体的操作，需要加锁
Dirent *dirent_alloc(void) 
{
	lock_acquire(&mtx_dirent);
	ASSERT(!list_empty(&dirent_free_list));
	struct list_elem* node = list_pop(&dirent_free_list);
	Dirent *dirent = elem2entry(struct Dirent,dirent_tag,node);
	// TODO: 需要初始化dirent的睡眠锁
	list_remove(&dirent->dirent_tag);
	memset(dirent, 0, sizeof(Dirent));
	used_dirents += 1;
	dirent->mode = 0777;
	lock_release(&mtx_dirent);
	return dirent;
}

void dirent_dealloc(Dirent *dirent) 
{
	lock_acquire(&mtx_dirent);

	memset(dirent, 0, sizeof(Dirent));
	list_append(&dirent_free_list, &dirent->dirent_tag);
	used_dirents -= 1;

	lock_release(&mtx_dirent);
}

static char *skip_slash(char *p) 
{
	while (*p == '/') {
		p++;
	}
	return p;
}

void dget(Dirent *dirent) 
{
	int is_filled = 0;
	int i;
	dirent->refcnt += 1;
}

/**
 * @brief 将dirent的引用数减一
 */
void dput(Dirent *dirent) 
{
	dirent->refcnt -= 1;
}

static Dirent *get_parent_dirent(Dirent *dirent)
{

	FileSystem *fs = dirent->file_system;
	Dirent *ans = NULL;

	if (dirent == fs->root) {
		// 本级就是文件系统的根目录
		// 跨级问题当且仅当当前处于文件系统根目录
		if (fs->mountPoint != NULL) {
			ans = get_parent_dirent(fs->mountPoint);
		} else {
			printk("reach root directory. it\'s parent dirent is self!\n");
			ans = dirent;
		}
	} else {
		// 包括上一级是根目录的情况，默认只导向到根目录，不导向挂载点
		ans = dirent->parent_dirent;
	}

	return ans;
}

static int dir_lookup(FileSystem *fs, Dirent *dir, char *name, struct Dirent **file) 
{
	Dirent *child;
	struct list_elem* ret = dir->child_list.head.next;
	while (ret!=&dir->child_list.tail)
	{
		if (strncmp(name, child->name, MAX_NAME_LEN) == 0) {
			dget(child);

			*file = child;
			return 0;
		}
	}
	printk("dir_lookup: %s not found in %s\n", name, dir->name);
	return -ENOENT;
}

static Dirent *try_enter_mount_dir(Dirent *dir) 
{
	struct Dirent * dir1 = dir;
	if (dir1->head != NULL) {
		FileSystem *fs = find_fs_by(find_fs_of_dir, dir1);
		if (fs == NULL) {
			printk("load mount fs error on dir %s!\n", dir1->name);
		}
		dir1 = fs->root;

		return dir1;
	}
	return dir1;
}

void dget_path(Dirent *file) 
{
	if (file == NULL) {
		printk("dget_path: file is NULL!\n");
	}
	while (1) {
		dget(file);
		printk("dget_path\n");
		if (file == file->file_system->root) {//如果是根目录
			file = file->file_system->mountPoint;//找到挂载的父目录
			if (file == NULL)//直到根目录
				return;
		} else {//否则寻找根目录
			file = get_parent_dirent(file);
		}
	}
}

/**
 * @brief 逆序释放路径上（包括自己）的所有目录引用
 */
void dput_path(Dirent *file) 
{
	while (1) {
		dput(file);
		if (file == file->file_system->root) {
			file = file->file_system->mountPoint;
			if (file == NULL)
				return;
		} else {
			file = get_parent_dirent(file);
		}
	}
}

int walk_path(FileSystem *fs, char *path, Dirent *baseDir, Dirent **pdir, Dirent **pfile, char *lastelem, longEntSet *longSet) 
{
	char *p;
	char name[MAX_NAME_LEN];
	Dirent *dir, *file, *tmp;
	int r;

	// 计算初始Dirent
	if (path[0] == '/' || baseDir == NULL) {
		file = fs->root;
	} else {
		// 持有baseDir这个Dirent的进程同样持有自己上级所有目录的意向锁
		file = baseDir;
	}

	dget_path(file);

	path = skip_slash(path);
	dir = NULL;
	name[0] = 0;
	*pfile = 0;

	while (*path != '\0') {
		
		dir = file;
		p = path;

		// 1. 循环读取文件名
		while (*path != '/' && *path != '\0') {
			path++;
		}

		if (path - p >= MAX_NAME_LEN) {
			return -ENAMETOOLONG;
		}

		memcpy(name, p, path - p);
		name[path - p] = '\0';

		path = skip_slash(path);

		// 2. 检查目录的属性
		// 如果不是目录，则直接报错
		if (!is_directory(&dir->raw_dirent)) {
			// 中途扫过的一个路径不是目录
			// 放掉中途所有引用
			dput_path(dir);
			return -ENOTDIR;
		}

		tmp = try_enter_mount_dir(dir);
		if (tmp != dir) {
			dget(tmp);
			dir = tmp;
		}

		// 处理 "." 和 ".."
		if (strncmp(name, ".", 2) == 0) {
			// 维持dir和file不变
			continue;
		} else if (strncmp(name, "..", 3) == 0) {
			// 回溯需要放引用
			Dirent *old_dir = dir;

			// 先获取上一级目录，再放引用，防止后续引用无效
			dir = get_parent_dirent(dir);
			file = get_parent_dirent(file);

			if (dir != old_dir) { // 排除dir是根目录的情况（根目录的上一级目录是自己）
				dput(old_dir);
			}

			// 挂载的目录
			if (old_dir == old_dir->file_system->root && old_dir->file_system->mountPoint) {
				dput(old_dir->file_system->mountPoint);
			}
			continue;
		}

		// 3. 继续遍历目录
		if ((r = dir_lookup(fs, dir, name, &file)) < 0) {
			// printf("r = %d\n", r);
			// *path == '\0'表示遍历到最后一个项目了
			if (r == -ENOENT && *path == '\0') {
				if (pdir) {
					*pdir = dir;
				}

				if (lastelem) {
					strcpy(lastelem, name);
				}

				*pfile = 0;
			}

			// 失败时，回溯意向锁
			dput_path(dir);
			return r;
		}
	}
	
	tmp = try_enter_mount_dir(file);
	printk("walk_path\n");
	if (tmp != file) {
		dget(tmp);
		file = tmp;
	}
	
	if (pdir) {
		*pdir = dir;
	}
	*pfile = file;
	return 0;
}

Dirent* search_dir_tree(Dirent* parent,char *name)
{
	Dirent *file;
	struct list_elem* dir_node = parent->child_list.head.next;
	while (dir_node!=&parent->child_list.tail)
	{
		file = elem2entry(struct Dirent,dirent_tag,dir_node);
		if (strcmp(file->name, name) == 0)
		{
			break;
		}
		dir_node = dir_node->next;
	}
	return file;
}
