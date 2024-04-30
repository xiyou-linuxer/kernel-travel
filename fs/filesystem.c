static struct FileSystem fs[MAX_FS_COUNT];

// FS分配锁
struct mutex mtx_fs;

static Buffer *getBlock(FileSystem *fs, u64 blockNum, bool is_read) {
	assert(fs != NULL);

	if (fs->image == NULL) {
		// 是挂载了根设备，直接读取块缓存层的数据即可
		return bufRead(fs->deviceNumber, blockNum, is_read);
	} else {
		// 处理挂载了文件的情况
		Dirent *img = fs->image;
		FileSystem *parentFs = fs->image->file_system;
		int blockNo = fileBlockNo(parentFs, img->first_clus, blockNum);
		return bufRead(parentFs->deviceNumber, blockNo, is_read);
	}
}

/**
 * @brief 分配一个文件系统结构体
 */
void allocFs(struct FileSystem **pFs) {
	mtx_lock(&mtx_fs);

	for (int i = 0; i < MAX_FS_COUNT; i++) {
		if (fs[i].valid == 0) {
			*pFs = &fs[i];
			memset(&fs[i], 0, sizeof(FileSystem));
			fs[i].valid = 1;
			fs[i].get = getBlock;

			mtx_unlock(&mtx_fs);
			return;
		}
	}
	panic("No more fs to alloc!");
}

/**
 * @brief 释放一个文件系统结构体
 */
void deAllocFs(struct FileSystem *fs) {
	mtx_lock(&mtx_fs);
	fs->valid = 0;
	memset(fs, 0, sizeof(struct FileSystem));
	mtx_unlock(&mtx_fs);
}
