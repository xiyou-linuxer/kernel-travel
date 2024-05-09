#include <linux/block_device.h>
#include <fs/buf.h>
#include <linux/stdio.h>
#include <debug.h>
#include<linux/string.h>

struct list Buflist;

/*BufferGroup *bufferGroups[BGROUP_NUM];//以页面为单位的缓冲区数组指针*/
BufferDataGroup bufferData[BGROUP_NUM];
BufferGroup bufferGroups[BGROUP_NUM];


/*初始化缓冲区管理*/
/*void bufInit() {
	printk("bufInit start\n");
	list_init(&Buflist);
	for (int i = 0; i < BGROUP_NUM; i++)
	{
		bufferGroups[i] = (BufferGroup *)sys_malloc(sizeof(struct BufferGroup));//动态分配控制块
		list_init(&bufferGroups[i]->list,&bufferGroups[i]->BufferGroup_node);
		for (int j = 0; j < BGROUP_BUF_NUM; j++)
		{
			Buffer *buf = &bufferGroups[i]->buf[j];
			buf->dev = -1;
			buf->data = sys_malloc(BUF_SIZE);//为数据区动态分配内存
			
		}
		list_append(&Buflist,&buf->Buffer_node);
	}
	printk("bufInit down\n");
}*/

/*为被回写的页重新分配控制块*/
/*void BufferGroupAlloc(unsigned long group)
{
	bufferGroups[group] = (BufferGroup *)sys_malloc(sizeof(struct BufferGroup));//动态分配控制块
	for (int j = 0; j < BGROUP_BUF_NUM; j++)
	{
		Buffer *buf = &bufferGroups[group]->buf[j];
		buf->dev = -1;
		buf->data = sys_malloc(PAGE_SIZE);//为数据区动态分配内存
		list_append(&Buflist,&buf->Buffer_node);
	}
}*/

/*static Buffer *bufAlloc(unsigned int dev, unsigned long blockno) {
	unsigned long group = blockno & BGROUP_MASK;
	Buffer *buf;
	if (bufferGroups[group] == NULL)//如果这一组已经被回写到回磁盘，则重新分配内存
	{
		BufferGroupAlloc(group);
		buf = &bufferGroups[group]->buf[1];
		buf->dev = dev;
		buf->blockno = blockno;
		buf->valid = 0;
		buf->dirty = 0;
		buf->refcnt = 1;
		return buf;
	}
	unsigned long arg[2] = {dev,blockno};
	// 检查对应块是否已经被缓存
	struct list_elem * elem=list_traversal(&bufferGroups[group]->list,check_cached,(void *)arg);
	if(elem!=NULL)//如果已经被缓存
	{
		buf = elem2entry(Buffer, Buffer_node, elem);
		return buf;
	}
	struct list_elem * elem1=list_reverse(&bufferGroups[group]->list,check_uncached,(void *)arg);
	// 没有被缓存，找到最久未使用的缓冲区（LRU策略换出）
	if(elem1!=NULL)
	{
		buf = elem2entry(Buffer, Buffer_node, elem1);
		return buf;
	}
	printk("No Buffer Available!\n");
	return NULL;
}*/

/*释放内存缓冲区*/
/*void bufRelease(Buffer *buf) {
	buf->refcnt--;
	if (buf->refcnt == 0) {
		// 刚刚完成使用的缓冲区，放在链表头部晚些被替换
		unsigned long group = buf->blockno & BGROUP_MASK;
		list_remove(&buf->Buffer_node);
		list_append(&bufferGroups[group]->list,&buf->Buffer_node);
		list_remove(&bufferGroups[group]->BufferGroup_node);
		list_append(&Buflist,&bufferGroups[group]->BufferGroup_node);
	}
}*/

/*内核线程，检测内存使用状况，如果大部分内存被使用，就释放不常用的数据区将其回写至磁盘*/
/*void bufSync() 
{
	while (1)
	{
		if ()//内存占用率超过。。。就将不常用的缓冲数据回写到磁盘里
		{
			for (int i = 0; i < BGROUP_NUM/2; i++)//循环将1/2
			{
				struct list_elem * BG_node=list_tail(&Buflist);
				BufferGroup* BG = elem2entry(BufferGroup,BufferGroup_node,BG_node);
				unsigned int blockno=&BG->buf[0].blockno;
				for (int j = 0; j < BGROUP_BUF_NUM; j++)
				{
					Buffer *buf = &BG->buf[j];
					sys_free(buf->data);
				}
				unsigned long group = blockno & BGROUP_MASK;
				bufferGroups[group] = NULL;//将该组对应的号置空
				sys_free(BG);//
			}
		}else
		{
			//休眠
		}
	}
}*/
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

/*list_traversal的回调函数，用于寻找已有的块*/
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
	printk("No Buffer Available!\n");
	return NULL;
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

void bufSync(void) {

	printk( "begin sync all pages to disk!\n");
	for (int i = 0; i < BGROUP_NUM; i++) {
		BufferGroup *b = &bufferGroups[i];
		for (int j = 0; j < BGROUP_BUF_NUM; j++) {
			Buffer *buf = &b->buf[j];
			if (buf->valid && buf->dirty) {
				block_write(buf->blockno,1,buf->data,1);
				buf->dirty = false;
			}
		}
	}
	printk( "sync all pages to disk done!\n");
}
