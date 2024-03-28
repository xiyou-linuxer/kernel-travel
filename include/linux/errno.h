#ifndef _LINUX_ERRNO_H
#define _LINUX_ERRNO_H

#include <uapi/linux/errno.h>

#define EKERNEL_BASE	512
/**
 * 这些错误仅内核可见
 */
#define ERESTARTSYS	(EKERNEL_BASE + 1)
/**
 * 不是正确的IOCTRL命令
 */
#define ENOIOCTLCMD	 (EKERNEL_BASE + 2)
/**
 * 驱动请求稍后再试
 */
#define EPROBE_DEFER	(EKERNEL_BASE + 3)
/**
 * IO请求已经入队，完成后会发出通知
 */
#define EIOCBQUEUED		(EKERNEL_BASE + 4)

#endif
