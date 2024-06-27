/**
 * @brief 获取 inode 的模式（文件类型和权限）
 *
 * @param sb 文件系统的超级块
 * @param inode 目标 inode
 * @return uint32_t inode 的模式（文件类型和权限）
 */
uint32_t ext4_inode_get_mode(struct ext4_sblock *sb, struct ext4_inode *inode)
{
	uint32_t v = to_le16(inode->mode); // 获取并转换 inode 的 mode 字段

	// 如果文件系统是由 HURD 操作系统创建的，则获取高 16 位的 mode
	if (ext4_get32(sb, creator_os) == EXT4_SUPERBLOCK_OS_HURD) {
		v |= ((uint32_t)to_le16(inode->osd2.hurd2.mode_high)) << 16;
	}

	return v; // 返回完整的 mode 值
}

/**
 * @brief 设置 inode 的模式（文件类型和权限）
 *
 * @param sb 文件系统的超级块
 * @param inode 目标 inode
 * @param mode 要设置的模式（文件类型和权限）
 */
void ext4_inode_set_mode(struct ext4_sblock *sb, struct ext4_inode *inode,
			 uint32_t mode)
{
	inode->mode = to_le16((mode << 16) >> 16); // 设置并转换 mode 的低 16 位

	// 如果文件系统是由 HURD 操作系统创建的，则设置 mode 的高 16 位
	if (ext4_get32(sb, creator_os) == EXT4_SUPERBLOCK_OS_HURD)
		inode->osd2.hurd2.mode_high = to_le16(mode >> 16);
}