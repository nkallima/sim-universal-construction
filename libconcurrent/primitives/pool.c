#include <pool.h>

const int POOL_SIZE = 8192;

void init_pool(PoolStruct *pool, int obj_size) {
    pool->heap = getAlignedMemory(CACHE_LINE_SIZE, POOL_SIZE * obj_size);
    pool->obj_size = obj_size;
    pool->index = 0;
}

void *alloc_obj(PoolStruct *pool) {
    int offset;

    if (pool->index == POOL_SIZE-1) {
        int size = pool->obj_size;
        init_pool(pool, size);
    }

    offset = pool->index;
    pool->index += 1;
    return (void *)(pool->heap + (offset * pool->obj_size));
}

void free_last_obj(PoolStruct *pool, void *obj) {
    if (pool->index > 0) {
        pool->index -= 1;
    }
}

void rollback(PoolStruct *pool, int num_objs) {
    if (pool->index - num_objs >= 0)
        pool->index -= num_objs;
    else
        pool->index = 0;
}
