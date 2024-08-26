#ifndef _FS_PATH_H
#define _FS_PATH_H

#include <fs/fs.h>
#include <fs/mount.h>
#include <xkernel/types.h>

#define MAX_PATH_LEN 512 // 路径最大长度
//该结构体用来记录搜索过的文件路径
struct path_search_record
{
    char searched_path[MAX_PATH_LEN]; // 查找过程中的父路径
    struct Dirent *parent_dir;           // 文件或目录所在的直接父目录
    enum dirent_type file_type;        // 找到的是普通文件还是目录,找不到将为未知类型(DIRENT_UNKNOWN)
};

/*
 * 文件路径查找时的位掩码：
 *  - 在最后跟随符号链接
 *  - 要求是一个目录
 *  - 即使是不存在的文件也接受结尾斜杠
 *  - 内部“还有更多路径组件”的标志
 *  - 使用 dcache_lock 锁定完成查找
 *  - 目录项缓存不受信任；强制进行实际查找
 */
#define LOOKUP_FOLLOW		 1	// 跟随符号链接
#define LOOKUP_DIRECTORY	 2	// 要求路径必须是目录
#define LOOKUP_CONTINUE		 4	// 查找操作需要继续
#define LOOKUP_PARENT		16		// 查找操作目标是父目录
#define LOOKUP_REVAL		64		// 重新验证路径的有效性
enum {
	LAST_NORM,       // 普通文件或目录
	LAST_ROOT,       // 根目录
	LAST_DOT,        // 当前目录 "."
	LAST_DOTDOT,    // 父目录 ".."
	LAST_BIND        // 用于绑定挂载点
};

enum { MAX_NESTED_LINKS = 8 };

// open_intent 结构用于描述打开文件时的意图
struct open_intent {
	int	flags;       // 打开文件时的标志，如 O_RDONLY、O_WRONLY 等
	int	create_mode;  // 如果正在创建文件，这是用于设置文件模式（权限）的参数
	struct file *file;  // 与打开的文件关联的 file 结构指针
};

// 定义 path 结构体，表示一个路径
struct path {
	struct vfsmount *mnt;  // 指向挂载点的指针
	struct Dirent *dentry;
	//struct dentry *dentry; // 指向目录项的指针
};

//路径查找中记录查找过程
struct nameidata {
	struct path	path;           // 包含当前查找路径的 vnode 和 vfsmount
	//struct qstr	last;           // 最后一个组件的名称和长度(dcache未实现)
	struct path	root;           // 查找操作的根路径
	unsigned int	flags;          // 查找相关的标志，如 LOOKUP_DIRECTORY
	int		last_type;      // 上一个路径组件的类型，如 LAST_ROOT
	unsigned	depth;          // 符号链接跟随的当前深度
	// 每个元素都是指针，指向一块内存
	char *saved_names[MAX_NESTED_LINKS + 1];  // 用于保存嵌套符号链接名称的数组

	/* Intent data */
	// 用于存储不同类型操作的特定数据
	union {
		struct open_intent open;  // 如果是打开文件操作，使用此结构
	} intent;
};

void path_resolution(const char* pathname);
char* path_parse(char* pathname, char* name_store);
int path_depth_cnt(char* pathname);
Dirent* search_dir_tree(Dirent* parent, char* name);
Dirent* search_file(const char *pathname, struct path_search_record *searched_record);
int filename2path(Dirent* file, char* newpath);
#endif