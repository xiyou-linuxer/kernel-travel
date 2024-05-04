#ifndef _FS_CHARDEV_H
#define _FS_CHARDEV_H

#include <types.h>

typedef struct chardev_data chardev_data_t;

// 读数据之前会调用的函数，用于获取数据
typedef void (*chardev_read_fn_t)(chardev_data_t *data);

// 写入数据之后会调用的函数，用于同步因为数据更改而产生的变更（比如执行一些内核动作）
typedef void (*chardev_write_fn_t)(chardev_data_t *data);

#define MAX_CHARDEV_STR_LEN                                                                        \
	(PAGE_SIZE - sizeof(u64) - sizeof(chardev_read_fn_t) - sizeof(chardev_write_fn_t))

typedef struct chardev_data {
	u64 size;
	chardev_read_fn_t read;
	chardev_write_fn_t write;
	char str[MAX_CHARDEV_STR_LEN];
} chardev_data_t;

void create_chardev_file(char *path, char *str, chardev_read_fn_t read, chardev_write_fn_t write);

#endif