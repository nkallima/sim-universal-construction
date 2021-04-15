#ifndef _CLHHASH_H_
#define _CLHHASH_H_

#include <stdint.h>
#include <primitives.h>
#include <types.h>
#include <pool.h>
#include <clh.h>

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

typedef struct CLHHash {
    HashOperations *announce;
    CLHLockStruct **synch CACHE_ALIGN;
    ptr_aligned_t *buckets;
    int size;
} CLHHash;

typedef struct CLHHashThreadState {
    PoolStruct pool;
} CLHHashThreadState;

inline void CLHHashInit(CLHHash *hash, int hash_size, int nthreads);
inline void CLHHashThreadStateInit(CLHHash *hash, CLHHashThreadState *th_state, int hash_size, int pid);
inline void CLHHashInsert(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int64_t value, int pid);
inline void CLHHashSearch(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid);
inline void CLHHashDelete(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid);

#endif
