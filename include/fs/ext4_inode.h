#ifndef EXT4_INODE_H_
#define EXT4_INODE_H_
// 定义 EXT4 文件系统中 inode 的文件类型模式

#define EXT4_INODE_MODE_FIFO 0x1000        // FIFO（命名管道）文件模式
#define EXT4_INODE_MODE_CHARDEV 0x2000     // 字符设备文件模式
#define EXT4_INODE_MODE_DIRECTORY 0x4000   // 目录文件模式
#define EXT4_INODE_MODE_BLOCKDEV 0x6000    // 块设备文件模式
#define EXT4_INODE_MODE_FILE 0x8000        // 常规文件模式
#define EXT4_INODE_MODE_SOFTLINK 0xA000    // 符号链接文件模式
#define EXT4_INODE_MODE_SOCKET 0xC000      // 套接字文件模式
#define EXT4_INODE_MODE_TYPE_MASK 0xF000   // 文件类型掩码，用于提取文件类型

#define EXT4_IS_DIR(mode) (((mode) & EXT4_INODE_MODE_TYPE_MASK) == EXT4_INODE_MODE_DIRECTORY)//判断是否是目录
#define EXT4_IS_FILE(mode) (((mode) & EXT4_INODE_MODE_TYPE_MASK) == EXT4_INODE_MODE_FILE)//判断是否是文件

#endif