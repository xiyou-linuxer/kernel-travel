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

#endif