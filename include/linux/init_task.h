#ifndef _LINUX_INIT_TASK_H
#define _LINUX_INIT_TASK_H

#include <linux/compiler_attributes.h>

#define __init_thread_info __section(".data..init_thread_info")

#endif /* _LINUX_INIT_TASK_H */
