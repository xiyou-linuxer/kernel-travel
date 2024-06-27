
#ifndef EXT4_DIR_H_
#define EXT4_DIR_H_
#include <fs/fs.h>
#include <fs/buf.h>
/*目录迭代器*/
struct ext4_dir_iter {
	struct Dirent *pdirent; /* 迭代器对应的目录项 */
	Buffer curr_blk;       /*< 当前数据块 */
	uint64_t curr_off;                /* 当前偏移量 */
	struct Dirent * *curr;         /* 当前目录项指针 */
};

#endif