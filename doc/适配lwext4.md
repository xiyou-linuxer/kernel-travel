# 适配lwext4

kernel-travel 在移植 lwext4 库内的代码时只移植了 ext4 文件系统相关的内容。对于上层的 VFS 层与底层的缓冲层都需要进行适配。

## lwext4 与 VFS 层之间的适配

### ext4文件系统的初始化

ext4 文件系统的初始化包括填充并检查超级块中的内容、构建目录树、将其挂载到 \ 目录下三步。

```c
void ext4_init(FileSystem* fs)
{
    printk("ext4 is initing...\n");
    strcpy(ext4Fs->name, "ext4");
    //初始化超级块信息
    ASSERT(fill_sb(ext4Fs) != 0);
    //初始化根目录
    ext4Fs->root->ext4_dir_en.inode = EXT4_ROOT_INO;//根目录对应的inode号
    strcpy(ext4Fs->root->name,"/");
    ext4Fs->root->file_system = ext4Fs;
    ext4Fs->op = &ext4_op;
    ext4Fs->root->type = DIRENT_DIR;
    //设置目录树的树根
    ext4Fs->root->parent_dirent = NULL; // 父节点为空，表示已经到达根节点
    list_init(&ext4Fs->root->child_list);
    ext4Fs->root->linkcnt = 1;
    //构建目录树
    build_dirent_ext4tree(ext4Fs->root);
    printk("\nbuild dirent tree succeed!\n");
}
```

其中 build_dirent_ext4tree 将 la-sdcard.img 中的 dirent 读取并连接到 VFS 层构建的目录树上。以方便后续 VFS 对路径解析。

### 目录结构间的适配

lwext4 库内提供了两种与目录相关的结构。

一种为ext4文件系统原生目录项结构 `ext4_dir_en` ,该结构用于对应文件的信息记录。该结构以匿名联合体的形式记录在 VFS 层的 Dirent 中。可以通过 Dirent 对该结构进行访问。

```c
/* 目录项结构 */
struct ext4_dir_en {
    uint32_t inode; /* 与目录项对应的inode号 */
    uint16_t entry_len; /*到下一个目录条目的距离*/
    uint8_t name_len;   /* 名称长度的低8位 */
    union ext4_dir_en_internal in;
    uint8_t name[]; /* 目录项的名字 */
};
```

一种为`ext4_dir`，该结构主要在读取目录项时用于记录目录项在目录内的偏移。

```c
/* 目录结构 */
struct ext4_dir {
    /** @brief 当前dir结构体对应的Dirent项 */
    Dirent *pdirent;
    /** @brief 当前目录项。 */
    Dirent *de;
    /** @brief 下一个目录项的偏移量。 */
    uint64_t next_off;
};
```

### 操作函数间的适配

在 VFS 层提供了操作函数结构体 `fs_operation` 。ext4将自己的操作函数注册在 `struct fs_operation ext4_op` 中，并通过 `ext4Fs` 调用注册在其中的函数。

```c
static const struct fs_operation ext4_op = {
    .fs_init_ptr = ext4_init,
    .file_read = ext4_fread,
    .file_write = ext4_fwrite
};
```

## lwext4 与缓冲层之间的适配

由于 lwext4 是以逻辑数据块为单位对磁盘进行读写，而缓冲层buf却是以物理扇区大小为单位对磁盘进行读写。因此要在这两层间进行适配。对于这两层间的转换，我们使用了两种类型的接口。

一种是针对逻辑数据块内的读写：

```c

int ext4_block_readbytes(uint64_t off, void *buf, uint32_t len)
{
    uint64_t block_idx;
    uint32_t blen;
    uint32_t unalg;
    int r = 0;
    int ph_bsize = 512;
    uint8_t* p = (void*)buf;

    // 计算起始块索引
    block_idx = (off / BUF_SIZE);
    // 处理第一个未对齐的块
    unalg = (off & (ph_bsize - 1));
    if (unalg) {

        // 计算读取长度
        uint32_t rlen = (ph_bsize - unalg) > len ? len : (ph_bsize - unalg);

        // 获取buf缓冲结构
        Buffer *buffer = bufRead(1, block_idx, 1);
        if (r != 0)
            return r;

        // 将数据复制到目标缓冲区
        memcpy(p, buffer->data->data + unalg, rlen);

        // 更新指针和剩余长度
        p += rlen;
        len -= rlen;
        block_idx++;
    }

    // 处理对齐的数据
    blen = len / ph_bsize;
    if (blen != 0) {
        // 读取对齐的块
        int count = 0;
        while (count < blen)
        {
            Buffer *buffer = bufRead(1, block_idx, 1);
            memcpy(p, buffer->data->data, ph_bsize);
            p += ph_bsize;
            len -= ph_bsize;
            block_idx++;
            count++;
        }
    }
    // 处理剩余的数据
    if (len) {
        // 读取最后一个块到临时缓冲区
        Buffer *buffer = bufRead(1, block_idx, 1);
        if (r != 0)
            return r;

        // 将数据复制到目标缓冲区
        memcpy(p, buffer->data->data + unalg, len);
    }
    return r;
}

```

一种为针对连续数据块的读写：

```c
/**
 * @brief 读取连续的扇区
 * @param 缓冲区 超级块指针。
 * @param lba 其实扇区号
 * @param cnt 连续读取的数量
 */

int ext4_blocks_get_direct(const void *buf,int block_size, uint64_t lba, uint32_t cnt)
{
    int count = 0;
    uint8_t* p = (void*)buf;
    int lp = block_size/BUF_SIZE;
    while (count < cnt)
    {
        int pba = lp*(lba+count);//计算逻辑块号
        for (int i = 0; i < lp; i++)//将块内的扇区都读出来
        {
            Buffer *buf = bufRead(1,pba+i,1);
            memcpy(p,buf->data->data,BUF_SIZE);
            p+=BUF_SIZE;
        }
        count++;
    }
    return count;
}
```

同时还提供了将逻辑块号转换为对应的扇区号的宏：

```c
#define EXT4_LBA2PBA(block_idx) (block_idx* (1024 << ext4Fs->superBlock.ext4_sblock.log_block_size)/PH_BLOCK_SIZE)
```
