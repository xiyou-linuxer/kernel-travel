#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/path.h>
#include <fs/mount.h>
#include <xkernel/sched.h>
#include <xkernel/list.h>
#include <xkernel/thread.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <fs/fs_struct.h>
#include <fs/file.h>
#include <debug.h>
/*路径处理相关*/
/*将路径转化为绝对路径，只支持 . 与 .. 开头的路径*/
void path_resolution(const char *pathname)
{
	char buf[MAX_NAME_LEN];
	struct task_struct *pthread = running_thread();
	if (pathname[0] == '/')//如果已经是绝对路径则直接返回
	{
		return ;
	}else if (pathname[0] == '.')
	{
		strcpy(buf,pthread->cwd);//将当前工作路径复制到buf中
		if (strcmp(buf,"/")==0)
		{
			strcat(buf,strchr(pathname,'/')+1);
		}else
		{
			strcat(buf,strchr(pathname,'/'));//再将除.之后的
		}
	}else if (pathname[0] == '..')
	{
		strcpy(buf,pthread->cwd);
		memset(strrchr(buf,"/"),0,MAX_NAME_LEN);//将最后一个/与他后面的内容清除
		strcat(buf,strchr(pathname,'/'));//再将传入路径除去 .. 后其他内容与buf拼接
	}else
	{
		//如果非以上三种符号开头则默认是目录/文件名，直接与当前工作目录拼接
		strcpy(buf,pthread->cwd);
		if (strcmp(buf,"/")!=0)
		{
			strcat(buf,"/");//当前工作目录不是根目录则先在路径上加上/再与传入的路径拼接
		}
		strcat(buf,pathname);
	}
	strcpy(pathname,buf);
}

char *path_parse(char *pathname, char *name_store)
{
	if (pathname[0] == '/')
	{ // 根目录不需要单独解析
		/* 路径中出现1个或多个连续的字符'/',将这些'/'跳过,如"///a/b" */
		while (*(++pathname) == '/')
		;
	}

	/* 开始一般的路径解析 */
	while (*pathname != '/' && *pathname != 0)
	{
		*name_store++ = *pathname++;
	}

	if (pathname[0] == 0)
	{ // 若路径字符串为空则返回NULL
		return NULL;
	}
	return pathname;
}

/* 返回路径深度,比如/a/b/c,深度为3 */
int path_depth_cnt(char *pathname)
{
	ASSERT(pathname != NULL);
	char *p = pathname;
	char name[MAX_NAME_LEN]; // 用于path_parse的参数做路径解析
	unsigned int depth = 0;

	/* 解析路径,从中拆分出各级名称 */
	p = path_parse(p, name);
	while (name[0])
	{
		depth++;
		memset(name, 0, MAX_NAME_LEN);
		if (p)
		{ // 如果p不等于NULL,继续分析路径
			p = path_parse(p, name);
		}
	}
	return depth;
}

Dirent* search_dir_tree(Dirent* parent,char *name)
{
	Dirent *file;
	struct list_elem* dir_node = parent->child_list.head.next;
	int ret = 0;
	
	while (dir_node!=&parent->child_list.tail)
	{
		file = elem2entry(struct Dirent,dirent_tag,dir_node);
		if (strcmp(file->name, name) == 0)
		{
			
			ret = 1;
			break;
		}
		dir_node = dir_node->next;
	}
	if (ret == 1)
	{
		return file;
	}else
	{
		return NULL;
	}
}

int filename2path(Dirent *file,char *newpath)
{
    Dirent *dir = file;
    char buf[MAX_PATH_LEN];
	strcpy(buf,file->name);
    while (strcmp(dir->parent_dirent->name,"/"))
    {
        strcat(buf,dir->parent_dirent->name);
        
        dir = dir->parent_dirent;
    }
    strcpy(newpath, "/");
    while (strchrs(buf,'/')!=0)
    {
        strcat(newpath,strrchr(buf,'/'));
		memset(strrchr(buf,'/'),0,MAX_NAME_LEN);
    }
    strcat(newpath,buf);
    return 1;
}

/**
 * @brief 搜索文件pathname
 * @param pathname 搜索路径
 * @param searched_record 用于记录搜索结果的结构体
 * @return 如果在树上找到结构体则返回Dirent,否则返回NULL
 */
Dirent* search_file(const char *pathname, struct path_search_record *searched_record)
{
	/* 如果待查找的是根目录,为避免下面无用的查找,直接返回已知根目录信息 */
	if (!strcmp(pathname, "/") || !strcmp(pathname, "/.") || !strcmp(pathname, "/.."))
	{
		searched_record->parent_dir = mnt_root.mnt_rootdir;
		searched_record->file_type = DIRENT_DIR;
		searched_record->searched_path[0] = 0; // 搜索路径置空
		return NULL;
	}

	uint32_t path_len = strlen(pathname);
	/* 保证pathname至少是这样的路径/x且小于最大长度 */
	ASSERT(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
	char *sub_path = (char *)pathname;
	Dirent *parent_dir = mnt_root.mnt_rootdir;
	Dirent *dir_e;
	/* 记录路径解析出来的各级名称,如路径"/a/b/c",
	* 数组name每次的值分别是"a","b","c" */
	char name[MAX_NAME_LEN] = {0};

	searched_record->parent_dir = parent_dir;
	searched_record->file_type = DIRENT_UNKNOWN;

	sub_path = path_parse(sub_path, name);
	while (name[0])
	{ // 若第一个字符就是结束符,结束循环
		/* 记录查找过的路径,但不能超过searched_path的长度512字节 */
		//ASSERT(strlen(searched_record->searched_path) < 512);
		/* 记录已存在的父目录 */
		strcat(searched_record->searched_path, "/");
		strcat(searched_record->searched_path, name);
		dir_e = search_dir_tree(parent_dir, name);
		/* 在所给的目录中查找文件 */
		if (dir_e != NULL)
		{
			memset(name, 0, MAX_NAME_LEN);
			/* 若sub_path不等于NULL,也就是未结束时继续拆分路径 */
			if (sub_path)
			{
				sub_path = path_parse(sub_path, name);
			}
			if (dir_e->type == DIRENT_DIR )// 如果被打开的是目录
			{ 
				if (dir_e->head!=NULL && dir_e->head!=dir_e->parent_dirent->head && dir_e->head->mnt_rootdir!=NULL)/* 判断目录下是否有挂载的文件系统，父目录与子目录对应的head若不同则说明子目录下挂载了其他文件系统*/
				{
					dir_e = dir_e->head->mnt_rootdir; //完成挂载点到被挂载文件系统的转换
				}
				searched_record->parent_dir = parent_dir;	
				parent_dir = dir_e;
				continue;
			}
			else if (dir_e->type == DIRENT_FILE)
			{ // 若是普通文件
				searched_record->file_type = DIRENT_FILE;
				return dir_e;
			}
		}
		else
		{
			return NULL;
		}
	}
	/* 执行到此,必然是遍历了完整路径并且查找的文件或目录只有同名目录存在 */
	/* 保存被查找目录的直接父目录 */
	//searched_record->parent_dir = dir_open(cur_part, parent_inode_no);
	searched_record->file_type = DIRENT_DIR;
	return dir_e;
}

/*设置目录查找的根*/
static __always_inline void set_root(struct nameidata *nd)
{
	// 如果 nameidata 结构中的根挂载点还未设置
	if (!nd->root.mnt) {
		// 获取当前进程的文件系统结构
		struct fs_struct* fs = &currentfs; // current->fs;

		// 获取读锁，保护文件系统结构的并发访问
		//read_lock(&fs->lock);
		lock_acquire(&fs->lock);

		// 将 nameidata 结构中的根路径设置为当前进程的根路径
		nd->root = fs->root;

        // 增加根路径的引用计数，防止其被释放
        path_get(&nd->root);
        
        // 释放读锁
		//read_unlock(&fs->lock);//未实现读写锁
		lock_release(&fs->lock);
    }
}

void path_get(struct path *path)
{
	/*mntget(path->mnt);
	dget(path->dentry);*/
	path->mnt->mnt_count++;
	path->dentry->refcnt++;
}

/**
 * 分为三种情况初始化路径查找
 * 1.路径以 / 开头
 * 2. dfd 指定为 AT_FDCWD 即从当前路径开始
 * 3. 指定了从 dfd 对应的路径开始
*/
static int path_init(int dfd, const char *name, unsigned int flags, struct nameidata *nd)
{
	int retval = 0; // 函数返回值，初始化为0（成功）
	int fput_needed; // 指示是否需要释放文件指针的变量
	struct file *file; // 文件结构的指针

	nd->last_type = LAST_ROOT; // 如果只有斜杠，初始化 last_type 为LAST_ROOT
	nd->flags = flags; // 设置 nameidata 结构的标志
	nd->depth = 0; // 初始化深度为0
	nd->root.mnt = NULL; // 初始化根挂载点为 NULL

	// 如果名称以 '/' 开头，使用根目录
	if (*name == '/') {
		set_root(nd); // 在 nameidata 结构中设置根目录
		nd->path = nd->root; // 将当前路径设置为根路径
		path_get(&nd->root); // 增加根路径的引用计数
	} else if (dfd == AT_FDCWD) { // 如果 dfd 是 AT_FDCWD（当前工作目录）
		struct fs_struct *fs = &currentfs;//current->fs; // 获取当前进程的文件系统结构
		lock_acquire(&fs->lock); // 获取读锁
		nd->path = fs->pwd; // 将当前路径设置为当前工作目录
		path_get(&fs->pwd); // 增加当前工作目录的引用计数
		lock_release(&fs->lock); // 释放读锁
	} else { // 否则，使用 dfd 指定的目录文件描述符
		struct Dirent *dentry;

		file = fget_light(dfd, &fput_needed); // 获取文件结构
		retval = -EBADF; // 设置默认错误返回值为 -EBADF（错误的文件描述符）
		if (!file) // 如果获取文件失败
		goto out_fail;

		dentry = file->f_path.dentry; // 获取文件路径的目录项

		retval = -ENOTDIR; // 设置默认错误返回值为 -ENOTDIR（不是目录）
		if (dentry->type == DIRENT_DIR) // 如果不是目录
			goto fput_fail;
		nd->path = file->f_path; // 设置当前路径为文件路径
		path_get(&file->f_path); // 增加文件路径的引用计数

		fput_light(file, fput_needed); // 释放文件指针
	}
	return 0; // 成功返回0

fput_fail:
	fput_light(file, fput_needed); // 释放文件指针
out_fail:
	return retval; // 返回错误码
}

static int path_walk(const char *name, struct nameidata *nd)
{
	struct path save = nd->path;
	int result;

	//current->total_link_count = 0;

	/* make sure the stuff we saved doesn't go away */
	path_get(&save);//将查找的当前路径放入，获取其对应的dirent项与vfsmnt

	result = link_path_walk(name, nd);//真正的路径查找函数
	if (result == -ESTALE) {
		/* nd->path had been dropped */
		//current->total_link_count = 0;
		nd->path = save;
		path_get(&nd->path);
		nd->flags |= LOOKUP_REVAL;
		result = link_path_walk(name, nd);
	}

	path_put(&save);

	return result;
}

static int do_path_lookup(int dfd, const char *name, unsigned int flags, struct nameidata *nd)
{
	// 初始化路径查找
	int retval = path_init(dfd, name, flags, nd);

	// 如果初始化成功，执行实际的路径遍历
	if (!retval)
		retval = path_walk(name, nd);

	// 如果查找成功并且启用了审计功能，审计结果的inode
	/*if (unlikely(!retval && !audit_dummy_context() && nd->path.dentry && nd->path.dentry->d_inode))
		audit_inode(name, nd->path.dentry);*/

	// 如果nd中有根路径，释放相关资源
	if (nd->root.mnt) {
		path_put(&nd->root);
		nd->root.mnt = NULL;
	}

	// 返回查找结果
	return retval;
}