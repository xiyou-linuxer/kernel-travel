#ifndef _VFS_MOUNT_H
#define _VFS_MOUNT_H
#include <fs/fs.h>
#include <xkernel/list.h>

#define MNT_NUM 10		//最多能挂载的数量
struct vfsmount {
	struct vfsmount *mnt_parent;		// 父文件系统，我们挂载在其上
	struct Dirent *mnt_mountpoint;		// 挂载点的目录项,位于父目录的目录上
	struct Dirent *mnt_root;			// 挂载文件系统的根目录
	struct list_head mnt_mounts;		// 子挂载点列表，挂载点以此为锚
	struct list_head mnt_child;			// 子挂载点列表中的挂载点
	const char *mnt_devname;			// 设备名称，例如 /dev/dsk/hda1
	int mnt_count;						// 挂载点的引用计数
	int mnt_expiry_mark;				// 标记是否已标记为过期
};
#endif