#ifndef _LINUX_FS_STRUCT_H
#define _LINUX_FS_STRUCT_H

#include <fs/path.h>
#include <sync.h>

extern struct fs_struct currentfs;//测试路径解析，凑合用,由fs_init初始化

// 有三个将VFS层和系统的进程紧密联系在一起，它们分别是file_struct, fs_struct, namespace(mnt_namespace)结构体。
// 和进程相关的结构体，描述了文件系统
// 由进程描述符中的 fs 域指向，包含文件系统和进程相关的信息。
struct fs_struct {
	int users;				// 用户数目
	//rwlock_t lock;		// 保护该结构体的锁（未实现读写锁）
	struct lock lock;		
	int umask;				// 掩码
	int in_exec;			// 当前正执行的文件
	// 当前工作目录(pwd)和根目录
	struct path root, pwd;	// 根目录路径和当前工作目录路径
};

//extern struct kmem_cache *fs_cachep;

/*extern void exit_fs(struct task_struct *);
extern void set_fs_root(struct fs_struct *, struct path *);
extern void set_fs_pwd(struct fs_struct *, struct path *);
extern struct fs_struct *copy_fs_struct(struct fs_struct *);
extern void free_fs_struct(struct fs_struct *);
extern void daemonize_fs_struct(void);
extern int unshare_fs_struct(void);*/

#endif /* _LINUX_FS_STRUCT_H */
