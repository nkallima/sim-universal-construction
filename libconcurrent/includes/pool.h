#ifndef _POOL_H_
#define _POOL_H_

#include <primitives.h>

typedef struct BlockObject {
    struct BlockObject *next;
} BlockObject;

typedef struct PoolBlockMetadata {
    uint32_t object_size;
    uint32_t entries;
    uint32_t free_entries;
    uint32_t cur_entry;
    struct PoolBlock *next;
    struct PoolBlock *back;
} PoolBlockMetadata;

typedef struct PoolBlock {
    PoolBlockMetadata metadata;
    char heap[];
} PoolBlock;

typedef struct PoolStruct {
    uint32_t obj_size;
    uint32_t entries_per_block;
    BlockObject *recycle_list;
    PoolBlock *head_block;
    PoolBlock *cur_block;
} PoolStruct;

#define POOL_BLOCK_METADATA     sizeof(PoolBlockMetadata)
#define POOL_INIT_ERROR         -1
#define POOL_INIT_SUCC          0
#define POOL_OBJECT_ALLOC_ERROR -1
#define POOL_OBJECTALLOC_SUCC   0

int init_pool(PoolStruct *pool, uint32_t obj_size);
void *alloc_obj(PoolStruct *pool);
void recycle_obj(PoolStruct *pool, void *obj);
void rollback(PoolStruct *pool, uint32_t num_objs);
void destroy_pool(PoolStruct *pool);

#endif
