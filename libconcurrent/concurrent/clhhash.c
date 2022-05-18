#include <clhhash.h>
#include <stdbool.h>
#include <limits.h>

static inline int64_t hash_func(CLHHash *hash, int64_t key);
static inline RetVal serialOperations(void *h, ArgVal dummy_arg, int pid);

static const int HT_INSERT = 0, HT_DELETE = 1, HT_SEARCH = 2;

inline void CLHHashStructInit(CLHHash *hash, int num_cells, int nthreads) {
    int i;

    hash->size = num_cells;
    hash->announce = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(HashOperations));
    hash->synch = synchGetAlignedMemory(CACHE_LINE_SIZE, num_cells * sizeof(CLHLockStruct *));
    hash->cells = synchGetAlignedMemory(CACHE_LINE_SIZE, num_cells * sizeof(ptr_aligned_t));
    for (i = 0; i < num_cells; i++) {
        hash->synch[i] = CLHLockInit(nthreads);
        hash->cells[i].ptr = NULL;
    }
}

inline void CLHHashThreadStateInit(CLHHash *hash, CLHHashThreadState *th_state, int num_cells, int pid) {
    synchInitPool(&th_state->pool, sizeof(HashNode));
}

static inline int64_t hash_func(CLHHash *hash, int64_t key) {
    return key % hash->size;
}

static inline RetVal serialOperations(void *h, ArgVal dummy_arg, int pid) {
    HashOperations arg;
    int64_t key;
    int64_t value;
    CLHHash *hash = (CLHHash *)h;
    ptr_aligned_t *cells;
    HashNode *top;

    arg = hash->announce[pid];
    key = arg.key;
    value = arg.value;
    cells = (ptr_aligned_t *)hash->cells;
    top = cells[arg.cell].ptr;

    if (arg.op == HT_INSERT) {
        bool found = false;
        HashNode *cur = top;

        while (cur != NULL && found == false) {
            if (cur->key == key) {
                found = true;
                break;
            }
            cur = cur->next;
        }
        if (found == false) {
            arg.node->key = key;
            arg.node->value = value;
            arg.node->next = top;
            cells[arg.cell].ptr = arg.node;
        }
        return true;
    } else if (arg.op == HT_DELETE) {
        bool found = false;
        HashNode *cur = top, *prev = top;

        while (cur != NULL && found == false) {
            if (cur->key == key) {
                found = true;
                break;
            }
            if (cur != top)
                prev = prev->next;
            cur = cur->next;
        }
        if (found) {
            if (top != prev)
                prev->next = cur->next;
            else
                cells[arg.cell].ptr = cur->next;
        }
        return found;
    } else { // SEARCH
        bool found = false;
        HashNode *cur = top;

        while (cur != NULL && found == false) {
            if (cur->key == key) {
                found = true;
                break;
            }
            cur = cur->next;
        }
        return found;
    }
}

inline bool CLHHashInsert(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int64_t value, int pid) {
    HashOperations args;
    RetVal ret;

    args.op = HT_INSERT;
    args.key = key;
    args.value = value;
    args.cell = hash_func(hash, key);
    args.node = synchAllocObj(&th_state->pool);
    hash->announce[pid] = args;

    CLHLock(hash->synch[args.cell], pid);
    ret = serialOperations((void *)hash, 0, pid);
    CLHUnlock(hash->synch[args.cell], pid);

    return ret;
}

inline RetVal CLHHashSearch(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;
    RetVal ret;

    args.op = HT_SEARCH;
    args.key = key;
    args.value = INT_MIN;
    args.cell = hash_func(hash, key);
    args.node = synchAllocObj(&th_state->pool);
    hash->announce[pid] = args;

    CLHLock(hash->synch[args.cell], pid);
    ret = serialOperations((void *)hash, 0, pid);
    CLHUnlock(hash->synch[args.cell], pid);

    return ret;
}

inline void CLHHashDelete(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;

    args.op = HT_DELETE;
    args.key = key;
    args.value = INT_MIN;
    args.cell = hash_func(hash, key);
    args.node = synchAllocObj(&th_state->pool);
    hash->announce[pid] = args;

    CLHLock(hash->synch[args.cell], pid);
    serialOperations((void *)hash, 0, pid);
    CLHUnlock(hash->synch[args.cell], pid);
}
