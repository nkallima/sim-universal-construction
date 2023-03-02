/// @file pool.h
/// @brief This file exposes a simple API for implementing a very simple pool object.
/// This pool object gives user the ability to allocate small chunks of memory (e.g. allocating nodes for using them in an queue or stack implementation)
/// in a fast and efficient way. The main purpose of this pool implementation is to add minimal overheads while benchmarking concurrent data structures,
/// such as stacks. queues, etc. This object does not provide thread-safe methods for accessing, and thus each of the running threads should use its own
/// instance without directly accessing the pool of any other thread.
#ifndef _POOL_H_
#define _POOL_H_

#include <stdint.h>
#include <primitives.h>

/// @brief A struct for the block object.
typedef struct SynchBlockObject {
    /// @brief The first field of a block object is a pointer to the next allocated block (if any).
    struct SynchBlockObject *next;
} SynchBlockObject;

/// @brief The metadata information for a single block.
typedef struct SynchPoolBlockMetadata {
    /// @brief The size of each stored object.
    uint32_t object_size;
    /// @brief The total number of objects stored in the block.
    uint32_t entries;
    /// @brief The number of free blocks available in the block.
    uint32_t free_entries;
    /// @brief The first free block. This should be returned in the next call of synchAllocObj.
    uint32_t cur_entry;
    /// @brief The next block of objects.
    struct SynchPoolBlock *next;
    /// @brief The previous block of objects.
    struct SynchPoolBlock *back;
} SynchPoolBlockMetadata;

/// @brief This struct stores the metadata of the block and all the objects of the block.
typedef struct SynchPoolBlock {
    /// @brief The metadata of the block.
    SynchPoolBlockMetadata metadata;
    /// @brief The actual storage space of the block (i.e. the objects of the block).
    char heap[];
} SynchPoolBlock;

/// @brief PoolStruct stores an instance of the pool object.
typedef struct SynchPoolStruct {
    /// @brief The size of the stored object (in bytes).
    uint32_t obj_size;
    /// @brief The number of objects that each block of the pool contains.
    uint32_t entries_per_block;
    /// @brief A list with the recycled items.
    SynchBlockObject *recycle_list;
    /// @brief The head of the list of blocks, where each block stores a specific amount of objects.
    SynchPoolBlock *head_block;
    /// @brief The latest allocated block of objects.
    SynchPoolBlock *cur_block;
} SynchPoolStruct;

/// @brief This is returned in case of error while calling synchInitPool.
#define SYNCH_POOL_INIT_ERROR         -1
/// @brief This is returned in case of success while calling synchInitPool.
#define SYNCH_POOL_INIT_SUCC          0
/// @brief This is returned in case of error while calling synchAllocObj (usually system's memory is exhausted).
#define SYNCH_POOL_OBJECT_ALLOC_ERROR NULL


/// @brief This function initializes a pool with objects of size obj_size.
/// @param pool A pointer to the pool of objects.
/// @param obj_size The size of objects that the pool contains.
/// @return In case of success, synchInitPool returns SYNCH_POOL_INIT_SUCC. In case of error, synchInitPool returns SYNCH_POOL_INIT_ERROR.
int synchInitPool(SynchPoolStruct *pool, uint32_t obj_size);

/// @brief This function initializes a pool with objects of size obj_size.
/// @param pool A pointer to the pool of objects.
/// @return On success, a pointer to a free object is returned. Otherwise, SYNCH_POOL_OBJECT_ALLOC_ERROR is returned.
void *synchAllocObj(SynchPoolStruct *pool);

/// @brief This function recycles the obj object for future use.
/// @param pool A pointer to the pool of objects.
/// @param obj A pointer to the object that should be recycled.
void synchRecycleObj(SynchPoolStruct *pool, void *obj);

/// @brief This function cancels the last num_objs consecutive object allocations. Note that no recycle_obj operation 
/// should have been called for any of the last num_objs consecutive object allocations.
/// @param pool A pointer to the pool of objects.
/// @param num_objs The number of consecutive allocations that should be canceled.
void synchRollback(SynchPoolStruct *pool, uint32_t num_objs);

/// @brief This function frees all the memory allocated by the pool object.
/// @param pool A pointer to the pool of objects.
void synchDestroyPool(SynchPoolStruct *pool);

#endif
