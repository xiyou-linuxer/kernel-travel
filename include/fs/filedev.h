#ifndef _FILE_DEVICE_H
#define _FILE_DEVICE_H

#include <types.h>

// 文件设备的抽象
// 只需要实现读写即可

// 访问之前需要先持有Dirent锁
typedef struct FileDev {
	int dev_id;
	char *dev_name;
	void *data; // 设备内储存的数据
	int (*dev_read)(struct Dirent *file, int user, u64 dst, uint off, uint n);
	int (*dev_write)(struct Dirent *file, int user, u64 src, uint off, uint n);
} FileDev;

#endif
