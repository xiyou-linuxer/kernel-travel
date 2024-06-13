#ifndef EXT4_BLOCKDEV_H_
#define EXT4_BLOCKDEV_H_
#include <xkernel/types.h>
struct ext4_blockdev_iface {
	/**@brief   Open device function
	 * @param   bdev block device.*/
	int (*open)(struct ext4_blockdev *bdev);

	/**@brief   Block read function.
	 * @param   bdev block device
	 * @param   buf output buffer
	 * @param   blk_id block id
	 * @param   blk_cnt block count*/
	int (*bread)(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id,
		     uint32_t blk_cnt);

	/**@brief   Block write function.
	 * @param   buf input buffer
	 * @param   blk_id block id
	 * @param   blk_cnt block count*/
	int (*bwrite)(struct ext4_blockdev *bdev, const void *buf,
		      uint64_t blk_id, uint32_t blk_cnt);

	/**@brief   Close device function.
	 * @param   bdev block device.*/
	int (*close)(struct ext4_blockdev *bdev);

	/**@brief   Lock block device. Required in multi partition mode
	 *          operations. Not mandatory field.
	 * @param   bdev block device.*/
	int (*lock)(struct ext4_blockdev *bdev);

	/**@brief   Unlock block device. Required in multi partition mode
	 *          operations. Not mandatory field.
	 * @param   bdev block device.*/
	int (*unlock)(struct ext4_blockdev *bdev);

	/**@brief   Block size (bytes): physical*/
	uint32_t ph_bsize;

	/**@brief   Block count: physical*/
	uint64_t ph_bcnt;

	/**@brief   Block size buffer: physical*/
	uint8_t *ph_bbuf;

	/**@brief   Reference counter to block device interface*/
	uint32_t ph_refctr;

	/**@brief   Physical read counter*/
	uint32_t bread_ctr;

	/**@brief   Physical write counter*/
	uint32_t bwrite_ctr;

	/**@brief   User data pointer*/
	void* p_user;
};

/**@brief   Definition of the simple block device.*/
struct ext4_blockdev {
	/**@brief Block device interface*/
	struct ext4_blockdev_iface *bdif;

	/**@brief Offset in bdif. For multi partition mode.*/
	uint64_t part_offset;

	/**@brief Part size in bdif. For multi partition mode.*/
	uint64_t part_size;

	/**@brief   Block cache.*/
	struct ext4_bcache *bc;

	/**@brief   Block size (bytes) logical*/
	uint32_t lg_bsize;

	/**@brief   Block count: logical*/
	uint64_t lg_bcnt;

	/**@brief   Cache write back mode reference counter*/
	uint32_t cache_write_back;

	/**@brief   The filesystem this block device belongs to. */
	struct ext4_fs *fs;

	void *journal;
};
#endif
