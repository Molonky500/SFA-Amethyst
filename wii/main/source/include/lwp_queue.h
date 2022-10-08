#ifndef __LWP_QUEUE_H__
#define __LWP_QUEUE_H__

#include <gctypes.h>

//#define _LWPQ_DEBUG

#ifdef _LWPQ_DEBUG
//extern int printk(const char *fmt,...);
#define printk exiPrintf
extern void exiPrintf(const char *fmt,...);
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _lwpnode {
	struct _lwpnode *next;
	struct _lwpnode *prev;
} lwp_node;

typedef struct _lwpqueue {
	lwp_node *first;
	lwp_node *perm_null;
	lwp_node *last;
} lwp_queue;

void __lwp_queue_initialize(lwp_queue *,void *,u32,u32);
lwp_node* __lwp_queue_get(lwp_queue *);
void __lwp_queue_append(lwp_queue *,lwp_node *);
void __lwp_queue_extract(lwp_node *);
void __lwp_queue_insert(lwp_node *,lwp_node *);

//#ifdef LIBOGC_INTERNAL
//#include <libogc/lwp_queue.inl>
//#endif

static __inline__ lwp_node* __lwp_queue_head(lwp_queue *queue)
{
	return (lwp_node*)queue;
}

static __inline__ lwp_node* __lwp_queue_tail(lwp_queue *queue)
{
	return (lwp_node*)&queue->perm_null;
}

static __inline__ u32 __lwp_queue_istail(lwp_queue *queue,lwp_node *node)
{
	return (node==__lwp_queue_tail(queue));
}

static __inline__ u32 __lwp_queue_ishead(lwp_queue *queue,lwp_node *node)
{
	return (node==__lwp_queue_head(queue));
}

static __inline__ lwp_node* __lwp_queue_firstnodeI(lwp_queue *queue)
{
	lwp_node *ret;
	lwp_node *new_first;
#ifdef _LWPQ_DEBUG
	printk("__lwp_queue_firstnodeI(%p)\n",queue);
#endif

	ret = queue->first;
	new_first = ret->next;
	queue->first = new_first;
	new_first->prev = __lwp_queue_head(queue);
	return ret;
}

static __inline__ void __lwp_queue_init_empty(lwp_queue *queue)
{
	queue->first = __lwp_queue_tail(queue);
	queue->perm_null = NULL;
	queue->last = __lwp_queue_head(queue);
}

static __inline__ u32 __lwp_queue_isempty(lwp_queue *queue)
{
	return (queue->first==__lwp_queue_tail(queue));
}

static __inline__ u32 __lwp_queue_onenode(lwp_queue *queue)
{
	return (queue->first==queue->last);
}

static __inline__ void __lwp_queue_appendI(lwp_queue *queue,lwp_node *node)
{
	lwp_node *old;
#ifdef _LWPQ_DEBUG
	printk("__lwp_queue_appendI(%p,%p)\n",queue,node);
#endif
	node->next = __lwp_queue_tail(queue);
	old = queue->last;
	queue->last = node;
	old->next = node;
	node->prev = old;
}

static __inline__ void __lwp_queue_extractI(lwp_node *node)
{
#ifdef _LWPQ_DEBUG
	printk("__lwp_queue_extractI(%p)\n",node);
#endif
	lwp_node *prev,*next;
	next = node->next;
	prev = node->prev;
	next->prev = prev;
	prev->next = next;
}

static __inline__ void __lwp_queue_insertI(lwp_node *after,lwp_node *node)
{
	lwp_node *before;

#ifdef _LWPQ_DEBUG
	printk("__lwp_queue_insertI(%p,%p)\n",after,node);
#endif
	node->prev = after;
	before = after->next;
	after->next = node;
	node->next = before;
	before->prev = node;
}

static __inline__ void __lwp_queue_prepend(lwp_queue *queue,lwp_node *node)
{
	__lwp_queue_insert(__lwp_queue_head(queue),node);
}

static __inline__ void __lwp_queue_prependI(lwp_queue *queue,lwp_node *node)
{
	__lwp_queue_insertI(__lwp_queue_head(queue),node);
}

static __inline__ lwp_node* __lwp_queue_getI(lwp_queue *queue)
{
	if(!__lwp_queue_isempty(queue))
		return __lwp_queue_firstnodeI(queue);
	else
		return NULL;
}

#ifdef __cplusplus
	}
#endif

#endif
