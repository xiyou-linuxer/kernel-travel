#include <linux/block_device.h>
#include <fs/buf.h>
#include <linux/stdio.h>
#include <debug.h>
#include<linux/string.h>

BufferDataGroup bufferData[BGROUP_NUM];
BufferGroup bufferGroups[BGROUP_NUM];

void bufInit() {
	printk("bufInit start\n");
	for (int i = 0; i < BGROUP_NUM; i++) {
		// 初始化缓冲区组
		BufferDataGroup *bdata = &bufferData[i];
		BufferGroup *b = &bufferGroups[i];
		// 初始化块缓冲区队列
		list_init(&b->list);
		for (int j = 0; j < BGROUP_BUF_NUM; j++) {
			// 初始化第 i 组的缓冲区
			Buffer *buf = &b->buf[j];
			buf->dev = -1;
			buf->data = &bdata->buf[j];
			// TODO: Init Lock
			list_append(&b->list,&buf->Buffer_node);
		}
	}
	printk("bufInit down\n");
}

/*list_traversal的回调函数*/
static int check_cached(struct list_elem* list_elem, void* arg)
{
	unsigned long * arg1 = (unsigned long *)arg;
	uint64_t dev = arg1[0];
	uint64_t blockno= arg1[1];
	Buffer* buf = elem2entry(Buffer, Buffer_node, list_elem);
	if (buf->dev == dev && buf->blockno == blockno) {
		buf->refcnt++;//引用计数加1
		return 1;
	}else
	{
		return 0;
	}
	
}

/*list_reverse的回调函数*/
static int check_uncached(struct list_elem* list_elem, void* arg)
{
	unsigned long * arg1 = (unsigned long *)arg;
	uint64_t dev = arg1[0];
	uint64_t blockno= arg1[1];
	Buffer* buf = elem2entry(Buffer, Buffer_node, list_elem);
	if (buf->refcnt == 0) {
		if (buf->valid && buf->dirty) {
			// 如果该缓冲区已经被使用，写回磁盘
			// 即换出时写回磁盘
			block_write(blockno,1,buf->data,1);;
		}
		// 如果该缓冲区没有被引用，直接使用
		buf->dev = dev;
		buf->blockno = blockno;
		buf->valid = 0;
		buf->dirty = 0;
		buf->refcnt = 1;
		return 1;
	}else
	{
		return 0;
	}
	
}

static Buffer *bufAlloc(u32 dev, u64 blockno) {
	u64 group = blockno & BGROUP_MASK;
	unsigned long arg[2] = {dev,blockno};
	// 检查对应块是否已经被缓存
	Buffer *buf;
	struct list_elem * elem=list_traversal(&bufferGroups[group].list,check_cached,(void *)arg);
	if(elem!=NULL)//如果已经被缓存
	{
		buf = elem2entry(Buffer, Buffer_node, elem);
		return buf;
	}
	struct list_elem * elem1=list_reverse(&bufferGroups[group].list,check_uncached,(void *)arg);
	// 没有被缓存，找到最久未使用的缓冲区（LRU策略换出）
	if(elem1!=NULL)
	{
		buf = elem2entry(Buffer, Buffer_node, elem1);
		return buf;
	}
	return NULL;
	printk("No Buffer Available!\n");
}

/**
 * @brief 当is_read为1时，首次获取时，只获取buffer，而不读取buffer，适合于clusterAlloc
 */
Buffer *bufRead(unsigned int dev, unsigned long blockno, bool is_read) {
	Buffer *buf = bufAlloc(dev, blockno);
	if (!buf->valid) {
		if (is_read) block_read(blockno,1,buf->data,1);
		buf->valid = true;
	}
	return buf;
}

/*标记为脏页等待统一回写*/
void bufWrite(Buffer *buf) {
	buf->dirty = true;
	return;
}

/*释放缓冲区*/
void bufRelease(Buffer *buf) {
	buf->refcnt--;
	if (buf->refcnt == 0) {
		// 刚刚完成使用的缓冲区，放在链表头部晚些被替换
		unsigned long group = buf->blockno & BGROUP_MASK;
		list_remove(&buf->Buffer_node);
		list_append(&bufferGroups[group].list,&buf->Buffer_node);
	}
}

void bufTest(unsigned long blockno) {
	printk("bufTest start\n");
	// 测试写入0号扇区（块）
	Buffer* b0 = bufRead(0, blockno, true);	
	for (int i = 0; i < BUF_SIZE; i++) {
		b0->data->data[i] = (u8)(blockno % 0xff) + i % 10;
	}
	b0->data->data[BUF_SIZE - 1] = 0;
	bufWrite(b0);
	Buffer b0_copy = *b0;
	bufRelease(b0);

	// 测试读出0号扇区
	b0 = bufRead(0, blockno, true);
	ASSERT(strncmp((const char *)b0->data, (const char *)b0_copy.data, BUF_SIZE) == 0);
	bufRelease(b0);
	printk("bufTest down\n");
}

/**
 * @brief 同步buf中所有的页到磁盘，同时把所有buf中的页都标记为非脏页
 */
void bufSync(void) {
	// 可以作为后台任务，由内核线程运行
	// 为了运行速度暂时关闭
	/*
	log(LEVEL_GLOBAL, "begin sync all pages to disk!\n");
	for (int i = 0; i < BGROUP_NUM; i++) {
		BufferGroup *b = &bufferGroups[i];
		for (int j = 0; j < BGROUP_BUF_NUM; j++) {
			Buffer *buf = &b->buf[j];
			if (buf->valid && buf->dirty) {
				disk_rw(buf, 1);
				buf->dirty = false;
			}
		}
	}
	log(LEVEL_GLOBAL, "sync all pages to disk done!\n");
	*/
}
