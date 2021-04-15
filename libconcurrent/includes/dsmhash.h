#ifndef _DSMHASH_H_
#define _DSMHASH_H_

#include <stdint.h>
#include <primitives.h>
#include <dsmsynch.h>
#include <pool.h>

typedef struct HashNode {
    struct HashNode *next;
    int64_t key;
    int64_t value;
} HashNode;

typedef struct HashOperations {
    int64_t key;
    int64_t value;
    HashNode *node;
    int32_t bucket;
    int32_t op;
} HashOperations;

typedef struct DSMHash {
    HashOperations *announce;
    DSMSynchStruct *synch CACHE_ALIGN;
    ptr_aligned_t *buckets;
    int size;
} DSMHash;

typedef struct DSMHashThreadState {
    DSMSynchThreadState *th_state;
    PoolStruct pool;
} DSMHashThreadState;

inline void DSMHashInit(DSMHash *hash, int hash_size, int nthreads);
inline void DSMHashThreadStateInit(DSMHash *hash, DSMHashThreadState *th_state, int hash_size, int pid);
inline void DSMHashInsert(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int64_t value, int pid);
inline void DSMHashSearch(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid);
inline void DSMHashDelete(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid);

#endif
