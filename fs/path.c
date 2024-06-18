#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/path.h>
#include <fs/mount.h>
#include <xkernel/sched.h>
#include <xkernel/list.h>
#include <xkernel/thread.h>
#include <xkernel/types.h>
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
		searched_record->parent_dir = mnt_root.mnt_root;
		searched_record->file_type = DIRENT_DIR;
		searched_record->searched_path[0] = 0; // 搜索路径置空
		return NULL;
	}

	uint32_t path_len = strlen(pathname);
	/* 保证pathname至少是这样的路径/x且小于最大长度 */
	ASSERT(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
	char *sub_path = (char *)pathname;
	Dirent *parent_dir = fatFs->root;
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
				if (dir_e->head!=NULL&&dir_e->head->mnt_root!=NULL)/* 判断目录下是否有挂载的文件系统 */
				{
					dir_e = dir_e->head->mnt_root; //完成挂载点到被挂载文件系统的转换
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