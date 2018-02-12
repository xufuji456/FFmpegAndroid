#include <stdio.h>
#include <malloc.h>

typedef struct queue_node {
	struct queue_node* prev;
	struct queue_node* next;
	void *p;//节点的值
} node;

// 表头
static node *phead = NULL;
static int count = 0;

static node* create_node(void *pval) {
	node *pnode = NULL;
	pnode = (node *) malloc(sizeof(node));
	if (pnode) {
		// 默认的，pnode的前一节点和后一节点都指向它自身
		pnode->prev = pnode->next = pnode;
		// 节点的值为pval
		pnode->p = pval;
	}
	return pnode;
}

/**
 *  创建双向链表
 * @return 成功，返回0；否则，返回-1。
 */
int create_queue() {
	phead = create_node(NULL);
	if (!phead) {
		return -1;
	}
	count = 0;
	return 0;
}

/**
 * 判断双向链表是否为空
 * @return 0为空
 */
int queue_is_empty() {
	return count == 0;
}

/**
 * 获取双向链表的大小
 * @return size
 */
int queue_size() {
	return count;
}

/**
 * 获取链表中指定位置的节点
 */

static node* get_node(int index) {
	if (index < 0 || index >= count) {
		return NULL;
	}
	if (index <= (count / 2)) {
		int i = 0;
		node *pnode = phead->next;
		while ((i++) < index)
			pnode = pnode->next;
		return pnode;
	}
	int j = 0;
	int rindex = count - index - 1;
	node *rnode = phead->prev;
	while ((j++) < rindex)
		rnode = rnode->prev;
	return rnode;
}

/**
 * 获取链表中指定位置的元素
 * @param index index
 * @return 指定位置的元素
 */
void* queue_get(int index) {
	node *pindex = get_node(index);
	if (!pindex) {
		return NULL;
	}
	return pindex->p;
}

/**
 * 获取链表中第1个元素的值
 * @return 第1个元素的值
 */
void* queue_get_first() {
	return queue_get(0);
}

/**
 * 获取链表中最后1个元素的值
 * @return 最后1个元素的值
 */
void* queue_get_last() {
	return queue_get(count - 1);
}

/**
 * 插入到index位置
 * @param index index
 * @param pval pval
 * @return 是否插入成功
 */
int queue_insert(int index, void* pval) {
	// 插入表头
	if (index == 0)
		return queue_insert_first(pval);
	// 获取要插入的位置对应的节点
	node *pindex = get_node(index);
	if (!pindex)
		return -1;
// 创建“节点”
	node *pnode = create_node(pval);
	if (!pnode)
		return -1;
	pnode->prev = pindex->prev;
	pnode->next = pindex;
	pindex->prev->next = pnode;
	pindex->prev = pnode;
	// 节点个数+1
	count++;
	return 0;
}

/**
 * 插入到链表头
 * @param pval
 * @return 是否插入成功
 */
int queue_insert_first(void *pval) {
	node *pnode = create_node(pval);
	if (!pnode)
		return -1;
	pnode->prev = phead;
	pnode->next = phead->next;
	phead->next->prev = pnode;
	phead->next = pnode;
	count++;
	return 0;
}

/**
 * 插入链表尾
 * @param pval
 * @return 是否插入成功
 */
int queue_append_last(void *pval) {
	node *pnode = create_node(pval);
	if (!pnode)
		return -1;
	pnode->next = phead;
	pnode->prev = phead->prev;
	phead->prev->next = pnode;
	phead->prev = pnode;
	count++;
	return 0;
}

/**
 * 删除双向链表中index位置的节点
 * @param index index
 * @return 成功，返回0；否则，返回-1
 */
int queue_delete(int index) {
	node *pindex = get_node(index);
	if (!pindex) {
		return -1;
	}
	pindex->next->prev = pindex->prev;
	pindex->prev->next = pindex->next;
	free(pindex);
	count--;
	return 0;
}

/**
 * 删除第一个节点
 */
int queue_delete_first() {
	return queue_delete(0);
}

/*
 * 删除最后一个节点
 */
int queue_delete_last() {
	return queue_delete(count - 1);
}

/**
 * 释放双向链表
 * @return 成功，返回0；否则，返回-1。
 */
int destroy_queue() {
	if (!phead) {
		return -1;
	}
	node *pnode = phead->next;
	node *ptmp = NULL;
	while (pnode != phead) {
		ptmp = pnode;
		pnode = pnode->next;
		free(ptmp);
	}
	free(phead);
	phead = NULL;
	count = 0;
	return 0;
}
