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

static void node2string(const  NODE_S *node,UINT32 replica_idx,CHAR *buf)
{
	snprintf(buf,127,"%s-%03d",node->iden,replica_idx);
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
		node2string(node,i,buff);
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
		node2string(node,i,buff);
		hash = conhash->hashfunc(buff);

		rbnode = util_rbtree_search(&(conhash->vnode_tree),hash);
		if(rbnode != NULL)
		{
			vnode = rbnode->data;
			if((vnode->hash == hash) && (vnode->node == node))
			{
				util_rbtree_delete(&(conhash->vnode_tree),rbnode);
				conhash_del_rbnode(rbnode);
			}
		}
	}
}/*static void conhash_del_replicas*/

CONHASH_S* conhash_init(conhash_hashfunc pfhash)
{
	CONHASH_S *conhash = (CONHASH_S *)malloc(sizeof(CONHASH_S));
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
	return conhash;
}

/* iden:node information */
/* replica:number of virtual node*/
void conhash_set_node(NODE_S *node,const CHAR *iden,UINT32 replica)
{
	strncpy(node->iden,iden,sizeof(node->iden)-1);
	node->replica = replica;

}/*void conhash_set_node*/

int conhash_add_node(CONHASH_S *conhash,NODE_S *node)
{
	if((conhash == NULL) || (node == NULL) || node->replica <= 0)
	{
		return -1;
	}

	conhash_add_replicas(conhash,node);

	return 0;
}
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
	}
	
	return NULL;
}

/*delete node*/
int conhash_del_node(CONHASH_S *conhash,NODE_S *node)
{
	if((conhash == NULL) || node == NULL)
		return -1;

	conhash_del_replicas(conhash,node);
}/*int conhash_del_node*/

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
}

