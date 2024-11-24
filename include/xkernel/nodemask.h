
#ifndef XKERNEL_NODEMARSK_H
#define XKERNEL_NODEMARSK_H
#include <asm/numa.h>

enum node_states {
    N_POSSIBLE,        /* 节点可能在未来的某个时间点上线 */
    N_ONLINE,          /* 节点当前是在线的，正积极参与系统运行 */
    N_NORMAL_MEMORY,   /* 节点拥有常规内存，可直接由CPU寻址 */

    /* 根据是否定义了CONFIG_HIGHMEM，N_HIGH_MEMORY的含义有所不同 */
#ifdef CONFIG_HIGHMEM
    N_HIGH_MEMORY,     /* 节点拥有常规内存或高内存（需通过特殊机制访问） */
#else
    N_HIGH_MEMORY = N_NORMAL_MEMORY, /* 如果没有定义CONFIG_HIGHMEM，则与N_NORMAL_MEMORY等价 */
#endif

    N_MEMORY,          /* 节点拥有内存，可以是常规的、高内存的，或者是可移动的 */
    N_CPU,             /* 节点拥有一个或多个CPU */
    N_GENERIC_INITIATOR,/* 节点拥有一个或多个通用发起者（具体含义取决于上下文） */
    NR_NODE_STATES     /* 标记枚举中状态值的数量，通常用于遍历或获取枚举大小 */
};

//遍历node节点
#define for_each_node_state(node, __state) \
	for ( (node) = 0; (node) == 0; (node) = 1)

#define for_each_online_node(node) for_each_node_state(node, N_ONLINE)
#endif