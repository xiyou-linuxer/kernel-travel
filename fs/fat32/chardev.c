#include <fs/chardev.h>
#include <fs/filedev.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/fat32.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/stdio.h>
#include <linux/errno.h>
#include <linux/memory.h>
#include <debug.h>
// reference: file_read
static int chardev_read(struct Dirent *file, int user, unsigned long dst, unsigned int off, unsigned int n) 
{
	chardev_data_t *pdata = file->dev->data;
	// 预读数据
	if (pdata->read) {
		pdata->read(pdata);
	}

	file->file_size = pdata->size; // 将文件大小写回到宿主文件上
	if (off >= pdata->size) {
		return -E_EXCEED_FILE;
	} else if (off + n > pdata->size) {
		printk("read too much. shorten read length from %d to %d!\n", n, pdata->size - off);
		n = pdata->size - off;
	}
	ASSERT(n != 0);

	if (user) {
		//copyOut(dst, (void *)(pdata->str + off), n);
	} else {
		memcpy((void *)dst, (void *)(pdata->str + off), n);
	}
	return n;
}

static int chardev_write(struct Dirent *file, int user, unsigned long src, unsigned int off, unsigned int n) 
{
	return -EINVAL;
}

/**
 * @brief 以字符串创建字符设备
 * @param str 初始字符串，可以为空
 * @param read 读数据之前会调用的函数，用于获取数据，可以为空
 * @param write
 * 写入数据之后会调用的函数，用于同步因为数据更改而产生的变更（比如执行一些内核动作），可以为空
 */
static struct FileDev *create_chardev(char *str, chardev_read_fn_t read, chardev_write_fn_t write) 
{
	/*struct FileDev *dev = (struct FileDev *)kmalloc(sizeof(struct FileDev));*/
	struct FileDev *dev = (struct FileDev *)get_kernel_pge();
	if (dev == NULL) {
		return NULL;
	}
	dev->dev_id = 'c';
	dev->dev_name = "char_dev";
	dev->dev_read = NULL/*chardev_read*/;
	dev->dev_write = NULL/*(int (*)(struct Dirent *, int, long unsigned int, unsigned int, unsigned int))chardev_write*/;

	chardev_data_t *p_data = (void *)get_kernel_pge();
	p_data->read = read;
	p_data->write = write;

	if (str == NULL) {
		str = "";
	}
	p_data->size = MIN(strlen(str), MAX_CHARDEV_STR_LEN);
	strcpy(p_data->str, str);
	dev->data = p_data;
	return dev;
}

void create_chardev_file(char *path, char *str, chardev_read_fn_t read, chardev_write_fn_t write) 
{
	extern FileSystem *fatFs;

	Dirent *file;
	if (getFile(fatFs->root, path, &file) < 0) {
		createFile(fatFs->root, path, &file);
	}
	if (file == NULL) {
		return;
	}
	file->dev = create_chardev(str, read, write);
	if (file->dev == NULL) {
		return;
	}
	file->type = DIRENT_CHARDEV;
	file_close(file);
}
