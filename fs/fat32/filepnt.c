#include <fs/cluster.h>
#include <linux/stdio.h>
#include <fs/fs.h>
#include <linux/types.h>
/**
 * @brief 更新pointer值
 */
void filepnt_setval(DirentPointer* fileptr, int i, int value) {
	unsigned int index1 = i / PAGE_NCLUSNO;
	unsigned int index2 = i % PAGE_NCLUSNO;
	if (index1 >= NDIRENT_SECPOINTER) {
		// 导入三级指针
		if (fileptr->third == NULL) {
			fileptr->third = (struct ThirdPointer *)kvmAlloc();
		}
		index1 = index1 - NDIRENT_SECPOINTER;
		if (fileptr->third->ptr[index1] == NULL) {
			fileptr->third->ptr[index1] = (struct TwicePointer *)kvmAlloc();
		}
		struct TwicePointer *twicep = fileptr->third->ptr[index1];
		twicep->cluster[index2] = value;
	} else {
		if (fileptr->second[index1] == NULL) {
			fileptr->second[index1] = (struct TwicePointer *)kvmAlloc();
		}
		struct TwicePointer *twicep = fileptr->second[index1];
		twicep->cluster[index2] = value;
	}
}

// 文件刚打开时初始化文件的指针
void filepnt_init(Dirent *file) {
	if (!file->pointer.valid) {
		// 首次打开，更新pointer
		int clus = file->first_clus;
		if (clus == 0) {
			// 文件的大小为0，返回
			return;
		}

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

/**
 * @brief 删除文件时清空文件的指针（主要是释放各种分配的内存）
 */
void filepnt_clear(Dirent *file) {
	DirentPointer *fileptr = &file->pointer;
	if (fileptr->valid) {
		for (int i = 0; i < NDIRENT_SECPOINTER; i++) {
			if (fileptr->second[i]) {
				kvmFree((u64)fileptr->second[i]);
			}
		}
		if (fileptr->third) {
			for (int i = 0; i < PAGE_SIZE / sizeof(long); i++) {
				if (fileptr->third->ptr[i]) {
					kvmFree((u64)fileptr->third->ptr[i]);
				}
			}
			kvmFree((u64)fileptr->third);
		}
	}
}

/**
 * @brief 返回文件file第fileClusNo块簇的簇号
 * @note 要求要查找的文件肯定有第fileClusNo个簇，否则会报错
 */
unsigned int filepnt_getclusbyno(Dirent *file, int fileClusNo) {
	unsigned int ind1 = fileClusNo / PAGE_NCLUSNO;
	unsigned int ind2 = fileClusNo % PAGE_NCLUSNO;
	DirentPointer *fileptr = &file->pointer;

	struct TwicePointer *twicep;
	if (ind1 < NDIRENT_SECPOINTER) {
		twicep = fileptr->second[ind1];
	} else {
		ind1 -= NDIRENT_SECPOINTER;
		assert(ind1 < PAGE_SIZE / sizeof(long));
		assert(fileptr->third != NULL);
		twicep = fileptr->third->ptr[ind1];
	}
	assert(twicep != NULL);
	u32 ret = twicep->cluster[ind2];
	assert(ret != 0); // 假设要查找的文件肯定有第fileClusNo个簇
	return ret;
}

/**
 * @brief 顺序释放文件的cluster
 * @param clus 第一个簇
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
