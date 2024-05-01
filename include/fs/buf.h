#ifndef _BUF_H
#define _BUF_H


#include <linux/types.h>
#include <param.h>
#include <linux/list.h>
#include <sync.h>

#ifdef FEATURE_LESS_MEMORY
#define BUF_SUM_SIZE (64 * 1024 * 1024) // 64MB
#else
#define BUF_SUM_SIZE (256 * 1024 * 1024) // 256MB
#endif

#define BUF_SIZE (512)			// 512B
#define BGROUP_NUM (1 << 14)		// 1024 * 8

#define BGROUP_MASK (BGROUP_NUM - 1)			      // 0x3ff
#define BGROUP_BUF_NUM (BUF_SUM_SIZE / BGROUP_NUM / BUF_SIZE) // 8
#define BUF_NUM (BUF_SUM_SIZE / BUF_SIZE)

/*缓冲区，对应一个扇区的大小*/
typedef struct BufferData {
	unsigned char data[BUF_SIZE];
} BufferData;

/*缓冲区组*/
typedef struct BufferDataGroup {
	BufferData buf[BGROUP_BUF_NUM];
} BufferDataGroup;

/*缓冲区控制块属性*/
typedef struct Buffer {
	unsigned long blockno;
	int dev;
	bool valid;
	bool dirty;
	unsigned short disk;
	unsigned short refcnt;
	BufferData *data;
	struct lock lock;
	struct list_elem Buffer_node; 	//链接到BufList中的节点
} Buffer;

/*缓冲区控制块组*/
typedef struct BufferGroup {
	struct list list; // 缓冲区双向链表（越靠前使用越频繁）
	Buffer buf[BGROUP_BUF_NUM];
	struct lock lock;
} BufferGroup;

void bufInit();
void bufTest(unsigned long blockno);

Buffer *bufRead(unsigned int dev, unsigned long blockno, bool is_read) __attribute__((warn_unused_result));
void bufWrite(Buffer *buf);
void bufRelease(Buffer *buf);
void bufSync();

#endif