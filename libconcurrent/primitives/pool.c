#include <unistd.h>

#include <config.h>
#include <pool.h>
#include <stdio.h>

#define POOL_BLOCK_METADATA_SIZE sizeof(SynchPoolBlockMetadata)

static const uint32_t BLOCK_SIZE = 4096 * 8192;

static void *get_new_block(uint32_t obj_size) {
    SynchPoolBlock *block;
    block = synchGetAlignedMemory(CACHE_LINE_SIZE, BLOCK_SIZE);
    block->metadata.entries = (BLOCK_SIZE - POOL_BLOCK_METADATA_SIZE) / obj_size;
    block->metadata.free_entries = (BLOCK_SIZE - POOL_BLOCK_METADATA_SIZE) / obj_size;
    block->metadata.cur_entry = 0;
    block->metadata.object_size = obj_size;
    block->metadata.next = NULL;
    block->metadata.back = NULL;

    return block;
}

int synchInitPool(SynchPoolStruct *pool, uint32_t obj_size) {
    SynchPoolBlock *block;

    if (obj_size > BLOCK_SIZE - POOL_BLOCK_METADATA_SIZE) {
        fprintf(stderr, "ERROR: synchInitPool: object size unsupported\n");

        return SYNCH_POOL_INIT_ERROR;
    } else if (obj_size < sizeof(void *)) {
        obj_size = sizeof(void *);
    }

    // Get the first block of the pool
    block = get_new_block(obj_size);

    pool->entries_per_block = (BLOCK_SIZE - POOL_BLOCK_METADATA_SIZE) / obj_size;
    pool->obj_size = obj_size;
    pool->recycle_list = NULL;
    pool->head_block = block;
    pool->cur_block = block;

    return SYNCH_POOL_INIT_SUCC;
}

void *synchAllocObj(SynchPoolStruct *pool) {
    SynchBlockObject *ret = NULL;

    if (pool->recycle_list == NULL) {
        if (pool->cur_block->metadata.free_entries > 0) {
            ret = (void *)&pool->cur_block->heap[(pool->cur_block->metadata.cur_entry) * (pool->obj_size)];
            pool->cur_block->metadata.free_entries -= 1;
            pool->cur_block->metadata.cur_entry += 1;
        } else {
            if (pool->cur_block->metadata.next != NULL) {
                pool->cur_block = pool->cur_block->metadata.next;
            } else {
                SynchPoolBlock *new_block = get_new_block(pool->obj_size);
                new_block->metadata.back = pool->cur_block;
                pool->cur_block = new_block;
            }
            ret = synchAllocObj(pool);
        }
    } else {
        ret = pool->recycle_list;
        pool->recycle_list = pool->recycle_list->next;
    }

#ifdef DEBUG
    if (ret == NULL) fprintf(stderr, "DEBUG: synchAllocObj returns a NULL object\n");
#endif

    return ret;
}

void synchRecycleObj(SynchPoolStruct *pool, void *obj) {
#ifndef SYNCH_POOL_NODE_RECYCLING_DISABLE
    SynchBlockObject *object = obj;
    object->next = pool->recycle_list;
    pool->recycle_list = object;
#endif
}

void synchRollback(SynchPoolStruct *pool, uint32_t num_objs) {
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

void synchDestroyPool(SynchPoolStruct *pool) {
    while (pool->head_block != NULL) {
        SynchPoolBlock *block = pool->head_block;
        pool->head_block = pool->head_block->metadata.next;
        synchFreeMemory(block, BLOCK_SIZE);
    }
    pool->head_block = NULL;
    pool->cur_block = NULL;
}
