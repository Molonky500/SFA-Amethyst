#ifndef __LWPNODE_H__
#define __LWPNODE_H__

typedef struct _lwpnode {
	struct _lwpnode *next;
	struct _lwpnode *prev;
} lwp_node;

#endif //__LWPNODE_H__
