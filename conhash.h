#ifndef _CONHASH_H_
#define _CONHASH_H_

#include "configure.h"
#include"util_rbtree.h"

#include <stdlib.h>

#define NODE_FLAG_INIT 0x01 /*node is init*/
#define NODE_FLAG_IN 0x02 /* node is added*/

struct node_s
{
	CHAR iden[64];/*node name*/
	UINT32 replica;/*number of replica virtual nodes*/
};
typedef struct node_s NODE_S;

struct virtual_node_s
{
	LONG hash;
	NODE_S *node; 
};/* virtual node structure */
typedef struct virtual_node_s VIRTUAL_NODE_S;

struct conhash_s
{
	util_rbtree_t vnode_tree; /* rbtree of virtual nodes */
  UINT32 ivnodes; /* virtual node number */
	LONG (*hashfunc)(const CHAR *);
};
typedef struct conhash_s CONHASH_S;

typedef LONG (*conhash_hashfunc)(const CHAR *instr);/*hash function*/
CONHASH_S* conhash_init(conhash_hashfunc pfhash, CHAR **ppcNodeIds,INT32 iNodeNums,UINT32 replica);
NODE_S* conhash_add_node(CONHASH_S *conhash,const CHAR *iden,UINT32 replica);



#endif

