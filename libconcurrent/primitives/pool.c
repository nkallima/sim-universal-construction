#include <unistd.h>

#include <config.h>
#include <pool.h>

static const uint32_t BLOCK_SIZE = 4096 * 8192;

static void *get_new_block(uint32_t obj_size) {
    PoolBlock *block;
    block = getAlignedMemory(CACHE_LINE_SIZE, BLOCK_SIZE);
    block->metadata.entries = (BLOCK_SIZE - POOL_BLOCK_METADATA) / obj_size;
    block->metadata.free_entries = (BLOCK_SIZE - POOL_BLOCK_METADATA) / obj_size;
    block->metadata.cur_entry = 0;
    block->metadata.object_size = obj_size;
    block->metadata.next = NULL;
    block->metadata.back = NULL;

    return block;
}

int init_pool(PoolStruct *pool, uint32_t obj_size) {
    PoolBlock *block;

    if (obj_size > BLOCK_SIZE - POOL_BLOCK_METADATA) {
        fprintf(stderr, "ERROR: init_pool: object size unsupported\n");

        return POOL_INIT_ERROR;
    } else if (obj_size < sizeof(void *)) {
        obj_size = sizeof(void *);
    }

    // Get the first block of the pool
    block = get_new_block(obj_size);

    pool->entries_per_block = (BLOCK_SIZE - POOL_BLOCK_METADATA) / obj_size;
    pool->obj_size = obj_size;
    pool->recycle_list = NULL;
    pool->head_block = block;
    pool->cur_block = block;

    return POOL_INIT_SUCC;
}

void *alloc_obj(PoolStruct *pool) {
    BlockObject *ret = NULL;

    if (pool->recycle_list == NULL) {
        if (pool->cur_block->metadata.free_entries > 0) {
            ret = (void *)&pool->cur_block->heap[(pool->cur_block->metadata.cur_entry) * (pool->obj_size)];
            pool->cur_block->metadata.free_entries -= 1;
            pool->cur_block->metadata.cur_entry += 1;
        } else {
            if (pool->cur_block->metadata.next != NULL) {
                pool->cur_block = pool->cur_block->metadata.next;
            } else {
                PoolBlock *new_block = get_new_block(pool->obj_size);
                new_block->metadata.back = pool->cur_block;
                pool->cur_block = new_block;
            }
            ret = alloc_obj(pool);
        }
    } else {
        ret = pool->recycle_list;
        pool->recycle_list = pool->recycle_list->next;
    }

#ifdef DEBUG
    if (ret == NULL) fprintf(stderr, "DEBUG: alloc_obj returns a NULL object\n");
#endif

    return ret;
}

void recycle_obj(PoolStruct *pool, void *obj) {
#ifndef POOL_NODE_RECYCLING_DISABLE
    BlockObject *object = obj;
    object->next = pool->recycle_list;
    pool->recycle_list = object;
#endif
}

void rollback(PoolStruct *pool, uint32_t num_objs) {
    while (num_objs > 0) {
        if (num_objs > pool->cur_block->metadata.cur_entry) {
            num_objs -= pool->cur_block->metadata.cur_entry;
            pool->cur_block->metadata.cur_entry = 0;
            pool->cur_block->metadata.free_entries = pool->cur_block->metadata.entries;
            if (pool->cur_block->metadata.back != NULL)
                pool->cur_block = pool->cur_block->metadata.back;
            else
                return;
        } else {
            pool->cur_block->metadata.cur_entry -= num_objs;
            pool->cur_block->metadata.free_entries += num_objs;
            num_objs = 0;
        }
    }
}

void destroy_pool(PoolStruct *pool) {
    while (pool->head_block != NULL) {
        PoolBlock *block = pool->head_block;
        pool->head_block = pool->head_block->metadata.next;
        freeMemory(block, BLOCK_SIZE);
    }
    pool->head_block = NULL;
    pool->cur_block = NULL;
}
