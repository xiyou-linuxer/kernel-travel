#ifndef _FS_CLUSTER_H
#define _FS_CLUSTER_H

#include <fs/fs.h>
#include <xkernel/types.h>

// 7个f，最高4位保留
#define FAT32_EOF 0xffffffful
#define FAT32_NOT_END_CLUSTER(cluster) ((cluster) < 0x0ffffff8ul)

// FarmOS 定义的函数
void clusterRead(FileSystem *fs, unsigned long cluster, long offset, void *dst, size_t n, bool isUser);
void clusterWrite(FileSystem *fs, unsigned long cluster, long offset, void *dst, size_t n, bool isUser);

unsigned long clusterAlloc(FileSystem *fs, unsigned long prevCluster) __attribute__((warn_unused_result));
void clusterFree(FileSystem *fs, unsigned long cluster, unsigned long prevCluster);

unsigned int fatRead(FileSystem *fs, unsigned long cluster);
void fatWrite(FileSystem *fs, unsigned long cluster, unsigned int content);
long fileBlockNo(FileSystem *fs, unsigned long firstclus, unsigned long fblockno);
 unsigned long clusterSec(FileSystem* fs, unsigned long cluster);
 void pre_read(struct Dirent* file, unsigned long dst, unsigned int n);
#endif