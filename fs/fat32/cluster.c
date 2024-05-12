#include <fs/buf.h>
#include <debug.h>
#include <fs/cluster.h>
#include <fs/fat32.h>
#include <linux/string.h>
#include <linux/stdio.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

unsigned long alloced_clus = 0;

// 簇的扇区号计算

/**
 * @return 返回簇号 cluster 所在的第一个扇区号
 */
 unsigned long clusterSec(FileSystem *fs, unsigned long cluster) {
	const int cluster_first_sec = 2;
	return fs->superBlock.first_data_sec +
	       (cluster - cluster_first_sec) * fs->superBlock.bpb.sec_per_clus;
}

/**
 * @return 返回簇号 cluster 所在的 FAT 表的扇区号
 * @param fatno FAT 表号，0 或 1
 */
static unsigned long clusterFatSec(FileSystem *fs, unsigned long cluster, unsigned char fatno) {
	const int fat32_entry_sz = 4;
	return fs->superBlock.bpb.rsvd_sec_cnt + fatno * fs->superBlock.bpb.fat_sz + cluster * fat32_entry_sz / fs->superBlock.bpb.bytes_per_sec;
}

/**
 * @return 返回簇号 cluster 在 FAT 表中的偏移量
 */
static unsigned long clusterFatSecIndex(FileSystem *fs, unsigned long cluster) {
	const int fat32_entry_sz = 4;
	return ((cluster * fat32_entry_sz) % fs->superBlock.bpb.bytes_per_sec) / fat32_entry_sz;
}

void clusterRead(FileSystem *fs, unsigned long cluster, long offset, void *dst, size_t n, bool isUser) {

	// 计算簇号 cluster 所在的扇区号
	unsigned long secno = clusterSec(fs, cluster) + offset / fs->superBlock.bpb.bytes_per_sec;
	// 计算簇号 cluster 所在的扇区内的偏移量
	unsigned long secoff = offset % fs->superBlock.bpb.bytes_per_sec;
	// 读扇区
	for (unsigned long i = 0; i < n; secno++, secoff = 0) {
		Buffer *buf = fs->get(fs, secno, true);
		// 计算本次读写的长度
		size_t len = min(fs->superBlock.bpb.bytes_per_sec - secoff, n - i);
		memcpy(dst + i, &buf->data->data[secoff], len);
		bufRelease(buf);
		i += len;
	}
}

void clusterWrite(FileSystem *fs, unsigned long cluster, long offset, void *src, size_t n, bool isUser) {
	(offset + n > fs->superBlock.bytes_per_clus);

	// 计算簇号 cluster 所在的扇区号
	unsigned long secno = clusterSec(fs, cluster) + offset / fs->superBlock.bpb.bytes_per_sec;
	// 计算簇号 cluster 所在的扇区内的偏移量
	unsigned long secoff = offset % fs->superBlock.bpb.bytes_per_sec;

	// 判断读写长度是否超过簇的大小
	//ASSERT(n > fs->superBlock.bytes_per_clus - offset);

	// 写扇区
	for (unsigned long i = 0; i < n; secno++, secoff = 0) {
		Buffer *buf = fs->get(fs, secno, true);
		// 计算本次读写的长度
		size_t len = min(fs->superBlock.bpb.bytes_per_sec - secoff, n - i);
		if (isUser) {
			/*extern void copyIn(unsigned long uPtr, void *kPtr, int len);
			copyIn((unsigned long)src + i, &buf->data->data[secoff], len);*/
		} else {
			memcpy(&buf->data->data[secoff], src + i, len);
		}
		bufWrite(buf);
		bufRelease(buf);
		i += len;
	}
}

void fatWrite(FileSystem *fs, unsigned long cluster, unsigned int content) {
	//ASSERT(cluster < 2 || cluster > fs->superBlock.data_clus_cnt + 1);

	// fatno从0开始
	for (unsigned char fatno = 0; fatno < fs->superBlock.bpb.fat_cnt; fatno++) {
		unsigned long fatSec = clusterFatSec(fs, cluster, fatno);
		Buffer *buf = fs->get(fs, fatSec, true);
		unsigned int *fat = (unsigned int *)buf->data->data;
		// 写入 FAT 表中的内容
		fat[clusterFatSecIndex(fs, cluster)] = content;
		bufWrite(buf);
		bufRelease(buf);
	}
}

unsigned int fatRead(FileSystem *fs, unsigned long cluster) {
	if (cluster < 2 || cluster > fs->superBlock.data_clus_cnt + 1) {
		printk("fatRead is 0! (cluster = %d)\n", cluster);
		return 0;
	}
	unsigned long fatSec = clusterFatSec(fs, cluster, 0);
	Buffer *buf = fs->get(fs, fatSec, true);

	if (buf == NULL) {
		printk("buf is NULL! cluster = %d\n", cluster);
	}

	unsigned int *fat = (unsigned int *)buf->data->data;
	// 读取 FAT 表中的内容
	unsigned int content = fat[clusterFatSecIndex(fs, cluster)];
	bufRelease(buf);
	return content;
}

/**
 * @brief 清空cluster
 */
static void clusterZero(FileSystem *fs, unsigned long cluster) {
	int n = fs->superBlock.bytes_per_clus;

	// 计算簇号 cluster 所在的扇区号
	unsigned long secno = clusterSec(fs, cluster);
	// 计算簇号 cluster 所在的扇区内的偏移量

	// 写扇区
	for (unsigned long i = 0; i < n; secno++) {
		Buffer *buf = fs->get(fs, secno, false);
		// 计算本次读写的长度
		size_t len = fs->superBlock.bpb.bytes_per_sec;
		memset(&buf->data->data[0], 0, len);
		bufWrite(buf);
		bufRelease(buf);
		i += len;
	}
}

/**
 * @brief 分配一个扇区，并将其内容清空
 */
unsigned long clusterAlloc(FileSystem *fs, unsigned long prev) {
	for (unsigned long cluster = prev == 0 ? 2 : prev + 1; cluster < fs->superBlock.data_clus_cnt + 2;
	     cluster++) {
		if (fatRead(fs, cluster) == 0) {
			if (prev != 0) {
				fatWrite(fs, prev, cluster);
			}
			fatWrite(fs, cluster, FAT32_EOF);
			clusterZero(fs, cluster);
			alloced_clus += 1;
			return cluster;
		}
	}
	return 0;
}

void clusterFree(FileSystem *fs, unsigned long cluster, unsigned long prev) {
	if (prev == 0) {
		fatWrite(fs, cluster, 0);
	} else {
		fatWrite(fs, prev, FAT32_EOF);
		fatWrite(fs, cluster, 0);
	}
}

/**
 * 计算文件的第 fblockno 个块所在的实际扇区号
 * @param  fs       该文件所在的文件系统
 * @param  firstclus 文件的第一个簇号
 * @param  fblockno 希望计算文件第几个块的扇区号
 */
long fileBlockNo(FileSystem *fs, unsigned long firstclus, unsigned long fblockno) {
	const unsigned long block_per_clus = fs->superBlock.bpb.sec_per_clus;
	// 找到第 fblockno 个块所在的簇号
	unsigned int curClus = firstclus;
	for (unsigned int i = 0; i < fblockno / block_per_clus; i++) {
		curClus = fatRead(fs, curClus);
		if (!FAT32_NOT_END_CLUSTER(curClus)) {
			printk("read mounted img error! exceed fileSize!\n");
			return -1;
		}
	}
	// 找到第 fblockno 个块所在的扇区号
	return clusterSec(fs, curClus) + fblockno % block_per_clus;
}
int countClusters(struct Dirent *file) {
	printk("count Cluster begin!\n");

	int clus = file->first_clus;
	int i = 0;
	if (clus == 0) {
		printk("cluster is 0!\n");
		return 0;
	}
	// 如果文件不包含任何块，则直接返回0即可。
	else {
		while (FAT32_NOT_END_CLUSTER(clus)) {
			printk("clus is %d\n", clus);
			clus = fatRead(file->file_system, clus);
			i += 1;
		}
		printk("count Cluster end!\n");
		return i;
	}
}

unsigned char checkSum(unsigned char *pFcbName) {
	short FcbNameLen;
	unsigned char Sum;
	Sum = 0;
	for (FcbNameLen = 11; FcbNameLen != 0; FcbNameLen--) {
		// NOTE: The operation is an unsigned char rotate right
		Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
	}
	return (Sum);
}