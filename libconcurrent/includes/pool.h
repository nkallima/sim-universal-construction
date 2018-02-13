#ifndef _POOL_H_
#define _POOL_H_

#include <primitives.h>

typedef struct HalfPoolStruct {
    char *heap;
    int index;
    int obj_size;
} HalfPoolStruct;


typedef struct PoolStruct {
    char *heap;
    int index;
    int obj_size;
    char pad[PAD_CACHE(sizeof(HalfPoolStruct))];
} PoolStruct;

void init_pool(PoolStruct *pool, int obj_size);
void *alloc_obj(PoolStruct *pool);
void free_last_obj(PoolStruct *pool, void *obj);
void rollback(PoolStruct *pool, int num_objs);

#endif
