//
// Created by ShiyuZhong in 2024/4/26.
//

#ifndef BASELINE_LRU_H
#define BASELINE_LRU_H

#endif //BASELINE_LRU_H
#include "stdio.h"
#include "assert.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"

#define TRUE 1
#define FALSE 0

struct LruNode {
    int key;
    int val;
    struct LruNode *next;
    struct LruNode *pre;
};

typedef struct LruNode node;

struct HashNode
{
    node *pnode;
    struct HashNode *next;
};

typedef struct HashNode hashnode;

typedef struct {
    int capacity;
    int cur_lru_num;
    node *LruList;//双向循环链表
    hashnode **nodetable;//hash表
} LRUCache;

hashnode **hashtable_init(int capacity);
hashnode *hash_node_init(node *pnode);
void delete_hash_node_intable(hashnode **pphashhead,const node *plrunode);
void insert_hash_node_intable(LRUCache *pObj,hashnode *pwaitadd);
void free_hash_list(hashnode **pphashhead);
node *lru_node_init(int key,int val);
void del_lru_node_by_node(node **pplisthead,node *pnode,bool delete);
void free_lru_list(node **lruhead);
node *find_lru_node_by_key(hashnode *phashnode,int key);
int lRUCacheGet(LRUCache* obj, int key);
void lRUCachePut(LRUCache* obj, int key, int value);
void lRUCacheFree(LRUCache* obj) ;
LRUCache* lRUCacheCreate(int capacity);