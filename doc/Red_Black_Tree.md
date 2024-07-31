
# 红黑树

红黑树：一种自平衡的二叉寻找树。
查找O(log n)、插入O(log n)和删除O(log n)。

## 前置知识

1. 什么是**自平衡**，什么是**二叉寻找树**。
2. 基本数据结构（链表），C语言基础。

### 自平衡

自平衡指的是，树在插入或者删除节点后，能够自动调整自身以维持树的平衡。平衡的树左右子树的高度差不会过大，这样可以保证查找、插入和删除操作的时间复杂度都维持在 O(log n)。

如果没有自平衡的，极端情况下树会变成一条单链表，效率不言而喻。
[极端情况下的非自平衡二叉树](./img/非自平衡二叉树.png)

常见的自平衡树有 AVL树，红黑树，B树。

### 二叉寻找树(BST, Binary Search Tree)

普通二叉树左右子节点没有任何要求，不要混淆这个点。跟数组和有序数组是一个道理。
BST 是一种特殊的二叉树，其中每个节点最多有两个子节点，分别称为左子节点和右子节点。BST 满足以下性质：

1. 对于每个节点，左子节点的值都小于该节点的值。
2. 对于每个节点，右子节点的值都大于该节点的值。

这种结构使得查找、插入和删除操作的时间复杂度在平均情况下是 O(log n)，但在最坏情况下（例如插入的节点是有序的），这些操作的时间复杂度会退化为 O(n)。

## 红黑树不同节点叫法

一张图清晰明了：
[name](./img/RBT_name.png)

Brother为兄弟节点，Parent为父节点，父节点的父节点为祖父节点，curr为当前节点。
叶子节点：叶子节点是当前红黑树上的所有NULL空节点。后续图为了简洁会省略叶子节点，请读者不要忘记叶子节点的存在。


## 红黑树的性质

既然为了追求高效的查找、插入和删除操作，自然有对应的性质（算法），下面是红黑树的五条性质：

1. 每个节点要不是红色，要不是黑色。
2. 根节点是黑色的。
3. 叶子节点（NULL）都是黑色的。
4. 每个红色节点的两个子节点都是黑色。
5. 任意一个节点到叶子节点的路径都包含数量相同的黑节点。

### 性质4

[性质4](./img/性质4_Err.png)

可以很明晰的看到，上图不满足性质四。红节点5 的 子节点3 还是红色。经过这么下面修改，就会发现它满足了所有性质：

[性质4_right](./img/性质4_right.png)

这里使用到了红黑树为了满足上述性质的操作：**变色**，当然还有其他操作，重新填色依旧不能解决极端情况下的单链表问题。

### 性质5

[性质5_right](./img/性质5_right.png)
上图满足性质五。

[性质5](./img/性质5_Err.png)
上图虽然和性质4的错误图很像，但是并不一样。在这个例子中，从根到左叶子的路径包含两个黑色节点（10 -> 5 -> 3），但从根到右叶子的路径仅包含一个黑色节点（10 -> 15 -> 12）。

这里不满足性质5,解决方式也很简单，使用上面提到的**变色**即可解决问题。

4 和 5 给出了 O(log n) 保证，因为 4 意味着你不能有两个路径中连续的红色节点，因此每个红色节点后面都有黑色。因此，如果 B 是每个简单路径上黑色节点的数量（根据5），那么由于4的最长可能路径是2B。

1).从第五条我们得知不能直接插入一个黑色节点，会造成黑色节点个数多一,所以插入操作我们都是插入的红色节点，插入后有问题再进行变色或者旋转。

2).如果在一个黑色节点上插入一个红色节点，不会破坏红黑树。在任何节点上插入一个红色节点只会违背性质4，不会直接违背性质5。

3).最长路径的节点个数不会超过最短路径的节点个数的二倍？

最短路径是全黑，最长路径是从根节点开始黑、红相间，黑红相间会使红节点和黑节点的数量一样多，就是最短全黑的2倍。

## struct rb_node 和 struct rb_root

```c
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
```
可能有读者好奇，为什么节点中没有对应的`val`。这样设计是为了通用性，举个例子，下面是内核中虚拟内存管理使用到的结构体，里面成员`vm_rb`就是红黑树的一个节点。因为C语言结构体的内存布局，可以通过`vm_rb`的虚拟地址和它在`struct vm_area_struct`结构体中成员所在的位置，通过偏移量计算就可以得到该结构体的虚拟地址。 

```c
// 虚拟内存区域描述符
struct vm_area_struct {
	struct mm_struct * vm_mm;
	unsigned long vm_start;
	unsigned long vm_end;
	// vma 在 mm_struct->mmap 双向链表中的前驱节点和后继节点
	struct vm_area_struct *vm_next, *vm_prev;
	// vma 在 mm_struct->mm_rb 红黑树中的节点
	struct rb_node vm_rb;
	unsigned long vm_flags; 	/*指定内存映射方式*/
	struct file * vm_file;
	unsigned long vm_pgoff;
};
```
对内存没有了解的可以看下面的解析，有基础可以跳过。
前提：
- 64位系统
- 8字节对齐
定义一个结构体后，它内存布局是下面这样的：
```
0x0000: struct mm_struct * vm_mm        (8字节)
0x0008: unsigned long vm_start          (8字节)
0x0010: unsigned long vm_end            (8字节)
0x0018: struct vm_area_struct * vm_next (8字节)
0x0020: struct vm_area_struct * vm_prev (8字节)
0x0028: struct rb_node
    0x0028: struct rb_node * rb_parent  (8字节)
    0x0030: int rb_color                (4字节)
    0x0034: 填充 (padding)              (4字节)	# 八字节对齐，为了提升效率，具体不展开讲
    0x0038: struct rb_node * rb_right   (8字节)
    0x0040: struct rb_node * rb_left    (8字节)
0x0048: unsigned long vm_flags          (8字节)
0x0050: struct file * vm_file           (8字节)
0x0058: unsigned long vm_pgoff          (8字节)
```
得到一个`struct rb_node`且知道它是哪个结构的成员（这里是struct vm_area_struct），自然可以通过`struct rb_node`计算出来当前结构的地址（这里是将红黑树节点的地址减去 0x28 的偏移量）。

## 基本操作

红黑树除了前面已经提到的`变色`，还有左旋，右旋操作。变色的逻辑处理没有单列出函数，而是而插入，删除中进行的，后文在插入，删除时会讲，这里不单独列出。

### 左旋

作用：将某个节点及其右子节点进行旋转，以将**右子节点提升为新的父节点**，并将**原节点降为其左子节点**。

函数实现：
```c
static void __rb_rotate_left(struct rb_node *node, struct rb_root *root)
{
	/*保存右子节点*/
	struct rb_node *right = node->rb_right;
	/*这里不是 == 而是赋值
	  如果右子节点的左子节点不为空，将其作为当前节点的右子节点。*/
	if ((node->rb_right = right->rb_left))
		/*更新右子节点的左子节点的父节点为当前节点*/
		right->rb_left->rb_parent = node;
	right->rb_left = node;

	/*更新右子节点的父节点*/
	if ((right->rb_parent = node->rb_parent))
	{
		if (node == node->rb_parent->rb_left)
			node->rb_parent->rb_left = right;
		else
			node->rb_parent->rb_right = right;
	}
	/*当前节点是根节点，更新根节点为右子节点*/
	else
		root->rb_node = right;
	node->rb_parent = right;
}
```
看下面的图一目了然（当前节点是父节点的左节点，且右子节点的左子节点不为空，其他情况不演示）：
注：橙色表示节点中的`rb_parent`指向。
[流程图](./img/流程图.png)


`right`记录为当前节点的右子节点。假设最开始是图1,执行玩下面代码后到达图2：

```c
	struct rb_node *right = node->rb_right;
	if ((node->rb_right = right->rb_left))
		right->rb_left->rb_parent = node;
	right->rb_left = node;
```
执行完下面代码到达图3:
```c
	/*更新右子节点的父节点*/
	if ((right->rb_parent = node->rb_parent))
	{
		if (node == node->rb_parent->rb_left)
			node->rb_parent->rb_left = right;
		else
			node->rb_parent->rb_right = right;
	}
```
最后到达图4:
```c
	node->rb_parent = right;
```
### 右旋

右旋同理，举一反三，不必多说，附上源码：
```c
static void __rb_rotate_right(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *left = node->rb_left;

	if ((node->rb_left = left->rb_right))
		left->rb_right->rb_parent = node;
	left->rb_right = node;

	if ((left->rb_parent = node->rb_parent))
	{
		if (node == node->rb_parent->rb_right)
			node->rb_parent->rb_right = left;
		else
			node->rb_parent->rb_left = left;
	}
	else
		root->rb_node = left;
	node->rb_parent = left;
}
```

## 红黑树的插入、删除、寻找

细心的读者应该发现了`struct rb_node`中没有比较的`val`，那么插入和删除该如何找到对应的节点位置？
Linux内核的红黑树实现仅仅实现了维护树的平衡性和颜色属性。至于需要插入到哪个节点，则是由调用者自行实现，换句话说：红黑树内部维护平衡性和颜色属性的代码中，根本不知道比较的`val`，这样极大的提高了代码的通用性，但是同时也要求调用者必须对操作的位置自行维护（即使调用者传入了不是预期的位置，红黑树对应代码也察觉不到）。红黑树的维护仅仅需要知道节点的颜色和节点之间的拓扑结构就够了。

### 插入

首先插入新的节点一定是红色节点。原因也很简单，反证法：假设目前有一棵正常的红黑树，如果插入一个黑色节点，必定会导致性质5（任意一个节点到叶子节点的路径都包含数量相同的黑节点）失效，为了高效和简单，所有插入的新节点都是红色的。

插入的逻辑分为：将节点链接到红黑树指定位置和修正树的平衡性和颜色属性。


#### 节点链接到红黑树指定位置

这部分的代码比较简单，直接看代码就行(注：这里也可以看到插入的节点一定是红色)：

```c
static inline void rb_link_node(struct rb_node * node, struct rb_node * parent,
				struct rb_node ** rb_link)
{
	node->rb_parent = parent;
	/*新插入的节点通常都是红色的*/
	node->rb_color = RB_RED;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}
```

#### 修正树的平衡性和颜色属性

插入一个节点后，红黑树调整如下：

1. 如果插入节点的父节点是黑色，而新节点是红色，直接插入，不做调整。
2. 如果插入节点的父节点和叔叔节点都是红色，则将父节点和叔叔节点变为黑色，将祖父节点变为红色。再将祖父节点（红色）当作新插入的节点继续检查，最多到根节点。
3. 如果插入节点的父节点是红色，叔叔节点是黑色，则需要旋转和变色操作共同来维护：
   1. 父节点是祖父节点的左子节点，而新节点是父节点的右子节点：需要一次左旋转，使新节点变为父节点，然后再对祖父节点进行右旋转。
   2. 父节点是祖父节点的左子节点，而新节点是父节点的左子节点：直接对祖父节点进行右旋转。
   3. 父节点是祖父节点的右子节点，而新节点是父节点的左子节点：需要一次右旋转，使新节点变为父节点，然后再对祖父节点进行左旋转。
   4. 父节点是祖父节点的右子节点，而新节点是父节点的右子节点：直接对祖父节点进行左旋转。

简单的记法：（拓扑）方向相同，祖父反方向（旋转）。在相同的基础上记着不同**先处理**新节点，再处理祖父节点，且这旋转两个方向**不同**就好了。

这就是插入逻辑的所有情况，最多需要旋转两次就可以重新修复红黑树的性质。左旋和右旋的基本操作在上面源码已经解析过了，这里直接调用，下面来看代码：
```c
void rb_insert_color(struct rb_node *node, struct rb_root *root)
{
	// parent 是父节点，gparent 是祖父节点，uncle 是叔叔节点
	struct rb_node *parent, *gparent;
	/*情况 1：不会进入 while 循环，父节点是黑色的*/
	while ((parent = node->rb_parent) && parent->rb_color == RB_RED)
	{	
		gparent = parent->rb_parent;
		if (parent == gparent->rb_left)
		{
			/*情况 2*/
			{
				register struct rb_node *uncle = gparent->rb_right;
				if (uncle && uncle->rb_color == RB_RED)
				{
					uncle->rb_color = RB_BLACK;	/*父节点和叔叔节点变为黑色*/
					parent->rb_color = RB_BLACK;
					gparent->rb_color = RB_RED;	/*祖父节点变为红色*/
					node = gparent;			/*祖父节点（红色）当作新插入的节点继续检查*/
					continue;
				}
			}
			/*情况 3.1*/
			if (parent->rb_right == node)
			{
				register struct rb_node *tmp;
				__rb_rotate_left(parent, root);	/*新节点变为父节点*/
				tmp = parent;
				parent = node;
				node = tmp;
			}
			/*情况 3.1 and 3.2*/
			parent->rb_color = RB_BLACK;
			gparent->rb_color = RB_RED;
			__rb_rotate_right(gparent, root);	/*祖父节点进行右旋转*/
		} else {
			/*情况 2*/
			{
				register struct rb_node *uncle = gparent->rb_left;
				if (uncle && uncle->rb_color == RB_RED)
				{
					uncle->rb_color = RB_BLACK;
					parent->rb_color = RB_BLACK;
					gparent->rb_color = RB_RED;
					node = gparent;
					continue;
				}
			}

			/*情况 3.3*/
			if (parent->rb_left == node)
			{
				register struct rb_node *tmp;
				__rb_rotate_right(parent, root);/*新节点变为父节点*/
				tmp = parent;
				parent = node;
				node = tmp;
			}

			/*情况 3.3 and 3.4*/
			parent->rb_color = RB_BLACK;
			gparent->rb_color = RB_RED;
			__rb_rotate_left(gparent, root);	/*祖父节点进行右旋转*/
		}
	}

	root->rb_node->rb_color = RB_BLACK;
}
```

### 删除

相对于插入的逻辑，删除处理的情况会更复杂，一步步理解，删除一个节点后，红黑树调整如下：
1. 删除节点没有子节点，直接删除当前节点。
2. 删除节点只有一个子节点，直接删除当前节点并将其子节点代替它的位置。
3. 删除节点有两个子节点，就不能直接使用子节点代替（位置不够），那么就要寻找右子树的最小节点，用后续节点代替要删除节点的位置。
4. 删除完后更新**后继节点的子节点、父节点的颜色属性**，如果**删除节点是黑色**的则需要旋转和重新着色。

看删除的代码逻辑：
```c
void rb_erase(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *child, *parent;
	int color;
	// 如果要删除的节点没有左子节点
	if (!node->rb_left)
		child = node->rb_right;
	// 如果要删除的节点没有右子节点
	else if (!node->rb_right)
		child = node->rb_left;
	else
	{
		// 如果要删除的节点有两个子节点
		struct rb_node *old = node, *left;

		// 找到右子树中最小的节点（后继节点）
		node = node->rb_right;
		while ((left = node->rb_left) != NULL)
			node = left;
		child = node->rb_right;
		parent = node->rb_parent;
		color = node->rb_color;

		// 更新后继节点的右子节点的父节点指针
		if (child)
			child->rb_parent = parent;
		// 更新后继节点的父节点指针
		if (parent)
		{
			if (parent->rb_left == node)
				parent->rb_left = child;
			else
				parent->rb_right = child;
		}
		else
			root->rb_node = child;
		
		// 如果后继节点的父节点是被删除节点
		if (node->rb_parent == old)
			parent = node;
		node->rb_parent = old->rb_parent;
		node->rb_color = old->rb_color;
		node->rb_right = old->rb_right;
		node->rb_left = old->rb_left;

		// 更新被删除节点的父节点的子节点指针
		if (old->rb_parent)
		{
			if (old->rb_parent->rb_left == old)
				old->rb_parent->rb_left = node;
			else
				old->rb_parent->rb_right = node;
		} else
			root->rb_node = node;

		// 更新被删除节点的左右子节点的父节点指针
		old->rb_left->rb_parent = node;
		if (old->rb_right)
			old->rb_right->rb_parent = node;
		goto color;
	}

	// 如果被删除的节点只有一个子节点或没有子节点
	parent = node->rb_parent;
	color = node->rb_color;

	// 更新子节点的父节点指针
	if (child)
		child->rb_parent = parent;
	// 更新父节点的子节点指针
	if (parent)
	{
		if (parent->rb_left == node)
			parent->rb_left = child;
		else
			parent->rb_right = child;
	}
	else
		root->rb_node = child;

 color:
	// 如果被删除的节点是黑色，修正红黑树的平衡性
	if (color == RB_BLACK)
		__rb_erase_color(child, parent, root);
}
```

红黑树通过旋转和变色来维持树的平衡，具体逻辑如下。
如果 node 是黑色且不是根节点，进入循环处理逻辑：
1. 如果 node 是父节点的左子节点：
   1. 兄弟节点是红色，通过旋转和颜色变换调整。
   2. 兄弟节点的子节点是黑色，通过颜色变换调整。
   3. 兄弟节点的右子节点是红色，通过旋转和颜色变换调整。
2. 如果 node 是父节点的右子节点：
   1. 兄弟节点是红色，通过旋转和颜色变换调整。
   2. 兄弟节点的子节点是黑色，通过颜色变换调整。
   3. 兄弟节点的左子节点是红色，通过旋转和颜色变换调整。
最后，设置 node 的颜色为黑色。看代码吧。

```c
static void __rb_erase_color(struct rb_node *node, struct rb_node *parent,
			     struct rb_root *root)
{
	struct rb_node *other;

	while ((!node || node->rb_color == RB_BLACK) && node != root->rb_node)
	{
		// 如果 node 是父节点的左子节点
		if (parent->rb_left == node)
		{
			other = parent->rb_right;
			// 兄弟节点是红色，通过旋转和颜色变换调整
			if (other->rb_color == RB_RED)
			{
				other->rb_color = RB_BLACK;
				parent->rb_color = RB_RED;
				__rb_rotate_left(parent, root);
				other = parent->rb_right;
			}
			// 兄弟节点的两个子节点是黑色，通过颜色变换调整
			if ((!other->rb_left ||
			     other->rb_left->rb_color == RB_BLACK)
			    && (!other->rb_right ||
				other->rb_right->rb_color == RB_BLACK))
			{
				other->rb_color = RB_RED;
				node = parent;
				parent = node->rb_parent;
			}
			// 兄弟节点的右子节点是红色，通过旋转和颜色变换调整
			else
			{
				// 兄弟节点的右子节点为黑色，通过右旋和颜色变换调整
				if (!other->rb_right ||
				    other->rb_right->rb_color == RB_BLACK)
				{
					register struct rb_node *o_left;
					if ((o_left = other->rb_left))
						o_left->rb_color = RB_BLACK;
					other->rb_color = RB_RED;
					__rb_rotate_right(other, root);
					other = parent->rb_right;
				}
				// 兄弟节点的右子节点为红色，通过左旋和颜色变换调整
				other->rb_color = parent->rb_color;
				parent->rb_color = RB_BLACK;
				if (other->rb_right)
					other->rb_right->rb_color = RB_BLACK;
				__rb_rotate_left(parent, root);
				node = root->rb_node;
				break;
			}
		}
		else
		{
			other = parent->rb_left;
			// 兄弟节点为红色，通过右旋和颜色变换调整
			if (other->rb_color == RB_RED)
			{
				other->rb_color = RB_BLACK;
				parent->rb_color = RB_RED;
				__rb_rotate_right(parent, root);
				other = parent->rb_left;
			}
			// 兄弟节点的两个子节点都是黑色，通过颜色变换调整
			if ((!other->rb_left ||
			     other->rb_left->rb_color == RB_BLACK)
			    && (!other->rb_right ||
				other->rb_right->rb_color == RB_BLACK))
			{
				other->rb_color = RB_RED;
				node = parent;
				parent = node->rb_parent;
			}
			// 兄弟节点的左子节点是红色，通过旋转和颜色变换调整
			else
			{
				// 兄弟节点的左子节点为黑色，通过左旋和颜色变换调整
				if (!other->rb_left ||
				    other->rb_left->rb_color == RB_BLACK)
				{
					register struct rb_node *o_right;
					if ((o_right = other->rb_right))
						o_right->rb_color = RB_BLACK;
					other->rb_color = RB_RED;
					__rb_rotate_left(other, root);
					other = parent->rb_left;
				}
				// 兄弟节点的左子节点为红色，通过右旋和颜色变换调整
				other->rb_color = parent->rb_color;
				parent->rb_color = RB_BLACK;
				if (other->rb_left)
					other->rb_left->rb_color = RB_BLACK;
				__rb_rotate_right(parent, root);
				node = root->rb_node;
				break;
			}
		}
	}
	if (node)
		node->rb_color = RB_BLACK;
}
```

### 寻找

能看懂插入和删除的逻辑，那么寻找的逻辑应该没什么难度，直接放代码吧，看两眼就懂了，比前面简单多了。
```c
/*获取红黑树的最小节点*/
struct rb_node *rb_first(struct rb_root *root)
{
	struct rb_node	*n;

	n = root->rb_node;
	if (!n)
		return NULL;
	while (n->rb_left)
		n = n->rb_left;
	return n;
}

/*获取红黑树的最大节点*/
struct rb_node *rb_last(struct rb_root *root)
{
	struct rb_node	*n;

	n = root->rb_node;
	if (!n)
		return NULL;
	while (n->rb_right)
		n = n->rb_right;
	return n;
}

/*获取给定节点在红黑树中的下一个节点*/
struct rb_node *rb_next(struct rb_node *node)
{
	if (node->rb_right) {
		node = node->rb_right; 
		while (node->rb_left)
			node=node->rb_left;
		return node;
	}

	while (node->rb_parent && node == node->rb_parent->rb_right)
		node = node->rb_parent;

	return node->rb_parent;
}

/*获取给定节点在红黑树中的前一个节点*/
struct rb_node *rb_prev(struct rb_node *node)
{
	if (node->rb_left) {
		node = node->rb_left; 
		while (node->rb_right)
			node=node->rb_right;
		return node;
	}

	while (node->rb_parent && node == node->rb_parent->rb_left) 
		node = node->rb_parent;

	return node->rb_parent;
}
```

最后十分推荐去参考资料中的红黑树操作动画中实际操作一下，理解会更深入。

# 参考资料

红黑树操作动画：https://www.cs.usfca.edu/~galles/visualization/RedBlack.html
红黑树属性：https://en.wikipedia.org/wiki/Rbtree
插入旋转理解：https://blog.csdn.net/m0_37707561/article/details/122967286
红黑树C实现源码（取之Linux 2.6实现）：
	https://github.com/xiyou-linuxer/kernel-travel/blob/master/lib/rbtree.c
	https://github.com/xiyou-linuxer/kernel-travel/blob/master/include/xkernel/rbtree.h