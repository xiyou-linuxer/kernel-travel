#ifndef _FS_BUF_H
#define _FS_BUF_H


#include <linux/types.h>
#include <linux/list.h>
#include <sync.h>
#include <asm/page.h>

#define BUF_SIZE (512)			// 512B
#define BGROUP_NUM (1 << 14)		// 1024 * 8
#define BUF_SUM_SIZE (64 * 1024 * 1024) // 64MB
#define BGROUP_MASK (BGROUP_NUM - 1)			      // 0x3ff
#define BGROUP_BUF_NUM (PAGE_SIZE / BUF_SIZE) // 8
#define BUF_NUM (BUF_SUM_SIZE / BUF_SIZE)

/*数据区，与磁盘块一一对应*/
typedef struct BufferData {
	unsigned char data[BUF_SIZE];
} BufferData;

/*缓冲区组*/
typedef struct BufferDataGroup {
	BufferData buf[BGROUP_BUF_NUM];
} BufferDataGroup;

/*缓冲区控制块属性，以块为单位*/
typedef struct Buffer {
	unsigned long blockno;			//控制块号
	int dev;
	bool valid;						//缓冲区的有效标志
	bool dirty;						//是否被写入的标志
	unsigned short disk;
	unsigned short refcnt;			//引用计数
	BufferData *data;				//缓冲区内存
	struct lock lock;				//缓冲控制块锁
	struct list_elem Buffer_node; 	//链接到BufList中的节点
} Buffer;

extern struct list Buflist;

/*以页面为单位的缓冲区控制块组*/
typedef struct BufferGroup {
	struct list list; // 缓冲区双向链表（越靠前使用越频繁）
	Buffer buf[BGROUP_BUF_NUM];
	struct lock lock;
	struct list_elem BufferGroup_node;
} BufferGroup;

void bufInit(void);
void bufTest(unsigned long blockno);

Buffer *bufRead(unsigned int dev, unsigned long blockno, bool is_read) __attribute__((warn_unused_result));
void bufWrite(Buffer *buf);
void bufRelease(Buffer *buf);
void bufSync(void);
#endif