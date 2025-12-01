#include "LRU.h"



//键值key的hash值计算函数
int hash_func(int capacity,int key)
{
    return key%capacity;
}

//hash表初始化
hashnode **hashtable_init(int capacity)
{
    hashnode **pnodetable = (hashnode **)malloc(sizeof(hashnode *)*capacity);
    memset(pnodetable,0,sizeof(hashnode *)*capacity);
    return pnodetable;
}

//hash桶下的节点hashnode的生成，使用lru双向链表的节点来初始化
hashnode *hash_node_init(node *pnode)
{
    hashnode *phashnode = (hashnode *)malloc(sizeof(hashnode));
    memset(phashnode,0,sizeof(hashnode));
    phashnode->pnode = pnode;
    return phashnode;
}

//由于可能会修改上层结点，所以使用二级指针
void delete_hash_node_intable(hashnode **pphashhead,const node *plrunode)
{
    if (!pphashhead || !(*pphashhead) || !plrunode)
    {
        return;
    }

    hashnode *prenode = NULL;
    hashnode *pcurnode = (*pphashhead);
    hashnode *nextnode = (*pphashhead)->next;

    //遍历找到待删除节点
    while (pcurnode && pcurnode->pnode != plrunode)
    {
        prenode = pcurnode;
        pcurnode = pcurnode->next;
    }

    if (prenode == NULL)//待删除节点为第一个结点
    {
        free(pcurnode);
        (*pphashhead) = nextnode;//删除第一个结点后，要将hash表头指针指向下一个结点（其可能为NULL）
    }
    else if (pcurnode == NULL)//待删除节点不存在
    {
        return;
    }
    else
    {
        prenode->next = pcurnode->next;
        pcurnode->next = NULL;
        free(pcurnode);
    }

    return;
}

//插入hash桶下的链表中，插入顺序没有要求，所以使用最适宜的头插法
void insert_hash_node_intable(LRUCache *pObj,hashnode *pwaitadd)
{
    if (!pObj || !pwaitadd)
    {
        return;
    }

    int index = hash_func(pObj->capacity, pwaitadd->pnode->key);
    pwaitadd->next = pObj->nodetable[index];
    pObj->nodetable[index] = pwaitadd;
}

//删除hash桶下的链表，输入参数为该链表的链表头，删除后要就将其置空，所以使用二级指针
void free_hash_list(hashnode **pphashhead)
{
    if (!pphashhead || !(*pphashhead))
    {
        return;
    }

    hashnode *phead = (*pphashhead);

    while (phead)
    {
        hashnode *ptmp = phead->next;
        free(phead);
        phead = ptmp;
    }

    (*pphashhead) = NULL;//置空
}

//由给定的键值对，生成一个lru双向循环链表中的节点
node *lru_node_init(int key,int val)
{
    node *pnode = (node *)malloc(sizeof(node));

    memset(pnode,0,sizeof(node));

    pnode->key = key;
    pnode->val = val;

    //生成链表后，使其前驱后继都指向自己
    pnode->next = pnode;
    pnode->pre = pnode;

    return pnode;
}

//头插入双向循环链表
void insert_lru_in_head(node **pplisthead,node *pnode)
{
    assert(pplisthead != NULL);
    assert(pnode != NULL);

    if ((*pplisthead) == NULL)
    {
        (*pplisthead) = pnode;
        return;
    }

    (*pplisthead)->pre->next = pnode;
    pnode->pre = (*pplisthead)->pre;

    pnode->next = (*pplisthead);
    (*pplisthead)->pre = pnode;

    (*pplisthead) = pnode;
}

//删除双向循环链表中的某个节点，由于有的情况是删除尾巴节点后将其插到头节点，所以这里使用一个参数bool delete来表示是否释放其内存，不释放内存的情况则是为了将该节点再次插入。
void del_lru_node_by_node(node **pplisthead,node *pnode,bool delete)
{
    if (!pnode || !pplisthead || !(*pplisthead))
    {
        return;
    }

    node *pnewhead = pnode->next;//先记录下待删除节点的下一个节点

    //pnode脱lru链
    pnode->pre->next = pnode->next;
    pnode->next->pre = pnode->pre;

    pnode->next = pnode->pre = pnode;//将pnode的指针初始化避免引起干扰

    if ((*pplisthead) == pnode)//要删除的是头节点，则需要更新lrulist的头指针
    {
        if ((*pplisthead) != pnewhead)
        {
            (*pplisthead) = pnewhead;
        }
        else
        {
            (*pplisthead) = NULL;//待删除的唯一的节点
        }
    }

    if (delete == true)
    {
        free(pnode);
    }

#if 0
    由lru中每个结点的初始化值可知，以上指针转向是安全的：
	头结点、尾结点、中间结点、只有一个结点，都是适用的
#endif
}

//释放双向循环链表
void free_lru_list(node **lruhead)
{
    if (!lruhead)
    {
        return;
    }

    node *phead = (*lruhead);

    while (phead)
    {
        node *ptmp = phead->next;
        if (ptmp == phead)
        {
            del_lru_node_by_node(lruhead,phead,TRUE);
            (*lruhead) = NULL;
            return;
        }

        del_lru_node_by_node(lruhead,phead,TRUE);
        phead = ptmp;
    }

    return;
}

//在hash桶下的链表中查找键值为key的lru双向循环链表的节点
node *find_lru_node_by_key(hashnode *phashnode,int key)
{
    while (phashnode)
    {
        if (phashnode->pnode->key == key)
        {
            return phashnode->pnode;
        }

        phashnode = phashnode->next;
    }

    return NULL;
}

//打印，调试接口
void lru_list_print(LRUCache* obj)
{
    node *ptmp = obj->LruList;
    node *head = ptmp;
    printf("ndoe num: %d\n",obj->cur_lru_num);

    while (ptmp)
    {
        printf("key: %d val: %d ------> ",ptmp->key,ptmp->val);
        node *pnext = ptmp->next;
        if (pnext == head)
        {
            break;
        }
        ptmp = pnext;
    }

    printf("\n\n");
}

//get操作
int lRUCacheGet(LRUCache* obj, int key) {
    int index = hash_func(obj->capacity,key);
    hashnode *phead = obj->nodetable[index];
    node *plrunode = find_lru_node_by_key(phead,key);

    if (plrunode == NULL)
    {
        return -1;
    }

    //找到此节点后，将其从原位置摘下,再头插入lru链表
    del_lru_node_by_node(&obj->LruList,plrunode,false);
    insert_lru_in_head(&obj->LruList, plrunode);

    return plrunode->val;
}

//put操作
void lRUCachePut(LRUCache* obj, int key, int value) {

    int index = hash_func(obj->capacity,key);
    hashnode *phashnode = obj->nodetable[index];

    if (phashnode == NULL)//hash表中index处的元素为空,那么lru链表中元素一定不存在
    {
        //由给定键值对,生成lru结点
        node *plrunode = lru_node_init(key,value);

        //生成该hash位置的第一个hash结点
        phashnode = hash_node_init(plrunode);

        obj->nodetable[index] = phashnode;

        //头插到lru链表中，但是要考虑lru链表的容积是否还够
        if (obj->cur_lru_num < obj->capacity)
        {
            insert_lru_in_head(&obj->LruList,plrunode);//将lru结点插入lru链表中
            ++obj->cur_lru_num;
        }
        else
        {
            //获取尾结点
            node *premove = obj->LruList->pre;

            //由尾结点的key值获取其hash桶位置
            int index = hash_func(obj->capacity,premove->key);

            hashnode *wait_del_hash_pos = obj->nodetable[index];//得到该结点所在的hash桶的头指针

            assert(wait_del_hash_pos != NULL);

            //从hash桶中清除此结点
            delete_hash_node_intable(&obj->nodetable[index],premove);

            //从lru中删除结点premove
            del_lru_node_by_node(&obj->LruList,premove,TRUE);

            //将新结点头插入lru链表中
            insert_lru_in_head(&obj->LruList, plrunode);

            //删掉一个结点后增加一个结点，结点数不变
        }
    }
    else
    {
        node *plrunode = find_lru_node_by_key(phashnode,key);
        if (plrunode)
        {
            //hash桶下的链表中,若节点存在，那么键相同,值覆盖
            plrunode->val = value;

            //从lru链表中摘下此节点
            del_lru_node_by_node(&obj->LruList,plrunode,FALSE);

            //将此节点头插入lru链表
            insert_lru_in_head(&obj->LruList, plrunode);
        }
        else
        {
            plrunode = lru_node_init(key,value);//由给定键值对,生成lru结点

            hashnode *ptmpnode = hash_node_init(plrunode);

            //头插到lru中，但是要考虑lru的容积是否还够
            if (obj->cur_lru_num < obj->capacity)
            {
                insert_lru_in_head(&obj->LruList,plrunode);//将lru结点插入到hash位置处的单向链表
                ++obj->cur_lru_num;
            }
            else
            {
                //获取尾结点
                node *premove = obj->LruList->pre;

                //由尾结点的key值获取其hash桶位置
                int index = hash_func(obj->capacity,premove->key);
                hashnode *wait_del_hash_pos = obj->nodetable[index];

                assert(wait_del_hash_pos != NULL);

                //从hash桶中清除此结点
                delete_hash_node_intable(&obj->nodetable[index],premove);

                //从lru中删除结点premove
                del_lru_node_by_node(&obj->LruList,premove,TRUE);

                //将新结点头插入lru链表中
                insert_lru_in_head(&obj->LruList, plrunode);

                //删掉一个结点后增加一个结点，结点数不变
            }

            //还必须插入hash表中
            insert_hash_node_intable(obj,ptmpnode);
        }
    }

    return;
}

//释放lru管理结构
void lRUCacheFree(LRUCache* obj) {
    if (obj)
    {
        //删除lru链表
        if (obj->capacity)
        {
            free_lru_list(&obj->LruList);
        }

        if (obj->nodetable)
        {
            //清理hash及其十字链表
            for (int i = 0;i < obj->capacity;++i)
            {
                free_hash_list(&obj->nodetable[i]);
            }

            free(obj->nodetable);
        }

        free(obj);
    }
}

//创建lru管理结构
LRUCache* lRUCacheCreate(int capacity) {
    if (capacity <= 0)
    {
        return NULL;
    }

    LRUCache *plru = (LRUCache *)malloc(sizeof(LRUCache));
    memset(plru,0,sizeof(LRUCache));
    plru->capacity = capacity;
    plru->nodetable = hashtable_init(capacity);
    return plru;
}
