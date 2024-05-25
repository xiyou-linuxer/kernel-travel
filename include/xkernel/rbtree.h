#ifndef _XKERNEL_RBTREE_H_
#define _XKERNEL_RBTREE_H_

#include <xkernel/compiler.h>

struct rb_node
{
	struct rb_node *rb_parent;
	int rb_color;
#define	RB_RED		0
#define	RB_BLACK	1
	struct rb_node *rb_right;
	struct rb_node *rb_left;
};

struct rb_root
{
	struct rb_node *rb_node;
};

#define	rb_entry(ptr, type, member) container_of(ptr, type, member)
#define RB_ROOT	(struct rb_root) { NULL, }

/*将一个节点 node 链接到红黑树中的指定位置
* 通常与 rb_insert_color() 一起使用
*/
static inline void rb_link_node(struct rb_node * node, struct rb_node * parent,
				struct rb_node ** rb_link)
{
	node->rb_parent = parent;
	/*新插入的节点通常都是红色的*/
	node->rb_color = RB_RED;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

/*插入节点并维护红黑树颜色*/
extern void rb_insert_color(struct rb_node *, struct rb_root *);
/*红黑树中删除节点，并且恢复红黑树性质*/
extern void rb_erase(struct rb_node *, struct rb_root *);

/*获取给定节点在红黑树中的下一个节点*/
extern struct rb_node *rb_next(struct rb_node *);
/*获取给定节点在红黑树中的前一个节点*/
extern struct rb_node *rb_prev(struct rb_node *);
/*获取红黑树的最小节点*/
extern struct rb_node *rb_first(struct rb_root *);
/*获取红黑树的最大节点*/
extern struct rb_node *rb_last(struct rb_root *);
/*将一个节点替换为另一个节点，并保持红黑树结构的完整性*/
extern void rb_replace_node(struct rb_node *victim, struct rb_node *new, 
			    struct rb_root *root);

#endif