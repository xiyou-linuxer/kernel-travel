#include <fs/cluster.h>
#include <fs/fs.h>
#include <fs/fat32.h>
#include <linux/types.h>
#include <linux/memory.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <debug.h>
void filepnt_setval(DirentPointer* fileptr, int i, int value) {
	unsigned int index1 = i / PAGE_NCLUSNO;
	unsigned int index2 = i % PAGE_NCLUSNO;
	if (index1 >= NDIRENT_SECPOINTER) {
		// 导入三级指针
		if (fileptr->third == NULL) {
			fileptr->third = (struct ThirdPointer *)get_page();
		}
		index1 = index1 - NDIRENT_SECPOINTER;
		if (fileptr->third->ptr[index1] == NULL) {
			fileptr->third->ptr[index1] = (struct TwicePointer *)get_page();
		}
		struct TwicePointer *twicep = fileptr->third->ptr[index1];
		twicep->cluster[index2] = value;
	} else {
		if (fileptr->second[index1] == NULL) {
			fileptr->second[index1] = (struct TwicePointer *)get_page();
		}
		struct TwicePointer *twicep = fileptr->second[index1];
		twicep->cluster[index2] = value;
	}
}
void filepnt_init(Dirent *file) {
	if (!file->pointer.valid) {
		// 首次打开，更新pointer
		int clus = file->first_clus;
		if (clus == 0) {
			// 文件的大小为0，返回
			return;
		}
		printk("filepnt_init\n");
		DirentPointer *fileptr = &file->pointer;
		for (int i = 0; FAT32_NOT_END_CLUSTER(clus); i++) {
			// 更新pointer值
			filepnt_setval(fileptr, i, clus);

			// 查询下一级簇号
			clus = fatRead(file->file_system, clus);
		}
		file->pointer.valid = 1;
	}
}
unsigned int filepnt_getclusbyno(Dirent *file, int fileClusNo) {

	unsigned int ind1 = fileClusNo / PAGE_NCLUSNO;
	unsigned int ind2 = fileClusNo % PAGE_NCLUSNO;
	DirentPointer *fileptr = &file->pointer;

	struct TwicePointer *twicep;
	if (ind1 < NDIRENT_SECPOINTER) {
		twicep = fileptr->second[ind1];
	} else {
		ind1 -= NDIRENT_SECPOINTER;
		ASSERT(ind1 < PAGE_SIZE / sizeof(long));
		ASSERT(fileptr->third != NULL);
		twicep = fileptr->third->ptr[ind1];
	}
	ASSERT(twicep != NULL);
	u32 ret = twicep->cluster[ind2];
	ASSERT(ret != 0); // 假设要查找的文件肯定有第fileClusNo个簇
	return ret;
}
/**
 * @brief 释放文件的簇
 * @param clus 文件的第一个簇号
*/
void clus_sequence_free(FileSystem *fs, int clus) {
	while (1) {
		int nxt_clus = fatRead(fs, clus);
		fatWrite(fs, clus, 0);
		clus = nxt_clus;

		if (!FAT32_NOT_END_CLUSTER(clus)) {
			break;
		}
	}
}