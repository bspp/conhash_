#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>

#include"conhash.h"
#include"md5.h"

static void conhash_md5_digest(const UCHAR *instr,UCHAR digest[16])
{
	md5_state_t md5_state;
	md5_init(&md5_state);
	md5_append(&md5_state,instr,strlen(instr));
	md5_finish(&md5_state,digest);
}/*void conhash_md5_digest md5信息摘要生成*/

LONG conhash_hash_fun_md5(const CHAR *instr)
{
	INT32 i = 0;
	LONG hash = 0;
	UCHAR digest[16];

	conhash_md5_digest(instr,digest);

	for( i = 0;i < 4; i++ )
	{
		hash += ((long)(digest[i*4 + 3]&0xFF) << 24) 
			| ((long)(digest[i*4 + 2]&0xFF) << 16) 
			| ((long)(digest[i*4 + 1]&0xFF) <<  8)  
			| ((long)(digest[i*4 + 0]&0xFF));
	}

	return hash;

}/*"LONG conhash_hash_fun_md5 获取哈希码"*/

static void metis_node2string(const  NODE_S *node,UINT32 replica_idx,CHAR *buf)
{
	snprintf(buf,127,"%s00%d",node->iden,replica_idx);
}

util_rbtree_node_t *conhash_get_rbnode(NODE_S *node,long hash)
{
	util_rbtree_node_t *rbnode;
	rbnode = (util_rbtree_node_t *)malloc(sizeof(util_rbtree_node_t));
	if( rbnode == NULL )
	{
		return NULL;
	}

	rbnode->key = hash;
	rbnode->data = malloc(sizeof(VIRTUAL_NODE_S));
	if(rbnode == NULL)
	{
		free(rbnode);
		rbnode = NULL;
	}

	VIRTUAL_NODE_S *vnode = rbnode->data;
	vnode->hash = hash;
	vnode->node = node;

	return rbnode;
}/*void conhash_get_rbnode*/

static void conhash_add_replicas(CONHASH_S *conhash,NODE_S *node)
{
	UINT32 i = 0;
	LONG hash;
	CHAR buff[128];

	if(conhash == NULL || node == NULL)
		return;

	util_rbtree_node_t *rbnode;

	for(;i < node->replica;i++)
	{
		metis_node2string(node,i,buff);
		hash = conhash->hashfunc(buff);

		//add virtual node
		if(util_rbtree_search(&(conhash->vnode_tree),hash) == NULL)
		{
			rbnode = conhash_get_rbnode(node,hash);
			if(rbnode != NULL)
			{
				util_rbtree_insert(&(conhash->vnode_tree),rbnode);
				conhash->ivnodes++;
			}
		}
	}
}/*static void replicas conhash_add_replicas*/

static void conhash_del_rbnode(util_rbtree_node_t *rbnode)
{
	VIRTUAL_NODE_S *node;
	node = rbnode->data;
	free(node);
	free(rbnode);
}

static void conhash_del_replicas(CONHASH_S *conhash,NODE_S *node)
{
	UINT32 i = 0;
	LONG hash = 0;
	char buff[128];

	VIRTUAL_NODE_S *vnode;
	util_rbtree_node_t *rbnode;

	for(i = 0;i < node->replica;i++)
	{
		metis_node2string(node,i,buff);
		hash = conhash->hashfunc(buff);

		rbnode = util_rbtree_search(&(conhash->vnode_tree),hash);
		if(rbnode != NULL)
		{
			vnode = rbnode->data;
			if((vnode->hash == hash) && (vnode->node == node))
			{
				util_rbtree_delete(&(conhash->vnode_tree),rbnode);
				conhash_del_rbnode(rbnode);
				conhash->ivnodes--;
			}
		}
	}
}/*static void conhash_del_replicas*/

CONHASH_S* conhash_init(conhash_hashfunc pfhash, CHAR **ppcNodeIds,INT32 iNodeNums,UINT32 replica)
{
	CONHASH_S *conhash = (CONHASH_S *)malloc(sizeof(CONHASH_S));
	INT32 i = 0;
	if(conhash == NULL)
	{
		return NULL;
	}
	//get callback function
	if(pfhash != NULL)
	{
		conhash->hashfunc = pfhash;
	}else
	{
		conhash->hashfunc = conhash_hash_fun_md5;
	}
	util_rbtree_init(&conhash->vnode_tree);
	if(conhash != NULL)
	{
		for(;i < iNodeNums;i++)
		{
			NODE_S *node =  conhash_add_node(conhash,ppcNodeIds[i],replica);
			if (node == NULL)
			{
				printf("Node %s add fail",ppcNodeIds);
				continue;
			}	
		}
	}

	return conhash;
}

/* iden:node information */
/* replica:number of virtual node*/
NODE_S* conhash_add_node(CONHASH_S *conhash,const CHAR *iden,UINT32 replica)
{ 
  if(conhash == NULL || replica <= 0)
	{
		printf("CONHASH is NULL or replica <= 0 \n");
		return NULL;
	}
	NODE_S *node = (NODE_S *)malloc(sizeof(NODE_S));
	if (node == NULL)
	{   
		return NULL;
	}   

	strncpy(node->iden,iden,sizeof(node->iden)-1);
	node->replica = replica;

	conhash_add_replicas(conhash,node);
	return node;

}/*NODE_S conhash_set_node*/

//int conhash_add_node(CONHASH_S *conhash,NODE_S *node)
//{
//	if((conhash == NULL) || (node == NULL) || node->replica <= 0)
//	{
//		return -1;
//	}
//
//	conhash_add_replicas(conhash,node);
//
//	return 0;
//}

NODE_S *conhash_get_node(CONHASH_S *conhash,CHAR *instr)
{
	char buf[128];
	LONG hash = 0;
	VIRTUAL_NODE_S *vnode;
	util_rbtree_node_t *rbnode;

	snprintf(buf,127,"%s000",instr);
	hash = conhash->hashfunc(buf);

	rbnode = util_rbtree_search(&(conhash->vnode_tree),hash);
	if(rbnode != NULL)
	{
		vnode = rbnode->data;
		return vnode->node;
	}else{
		printf("Not find\n");
	}
	return NULL;
}

/*delete node*/
void conhash_del_node(CONHASH_S *conhash,CHAR *instr)
{
	if(conhash == NULL)
		return;

	NODE_S *node = conhash_get_node(conhash,instr);
	if(node == NULL){
		return;
	}
	conhash_del_replicas(conhash,node);
	free(node);
	node = NULL;
}/*int conhash_del_node*/

void conhash_free_node(CONHASH_S *conhash)
{
  if(conhash == NULL)
			return;
	VIRTUAL_NODE_S *vnode;
	while(conhash->ivnodes > 0)
	{
		util_rbtree_node_t *root = conhash->vnode_tree.root;
		if(root != NULL)
		{
			vnode = root->data;
			conhash_del_replicas(conhash,vnode->node);
			free(vnode->node);
		}
	}
	free(conhash);
}

NODE_S* conhash_lookup(CONHASH_S *conhash,const CHAR *object)
{
	LONG hash;
	const util_rbtree_node_t *rbnode;

	if((conhash == NULL) || (conhash->ivnodes == 0) || (object) == NULL)
		return NULL;

	hash = conhash->hashfunc(object);

	rbnode = util_rbtree_lookup(&(conhash->vnode_tree),hash);

	if(rbnode != NULL)
	{
		VIRTUAL_NODE_S  *vnode = rbnode->data;
		return vnode->node;
	}
	return NULL;
}
int main(){
	char *a[]={"node1","node2","node3"};
	CONHASH_S *conhash = conhash_init(NULL,a,3,12);
	conhash_free_node(conhash);
}



























