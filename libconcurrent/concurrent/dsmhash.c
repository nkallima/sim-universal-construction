#include <dsmhash.h>

static inline int64_t hash_func(DSMHash *hash, int64_t key);
static inline RetVal serialOperations(void *h, ArgVal dummy_arg, int pid);

static const int HT_INSERT = 0, HT_DELETE = 1, HT_SEARCH = 2;

inline void DSMHashInit(DSMHash *hash, int hash_size, int nthreads) {
    int i;

    hash->size = hash_size;
    hash->announce = getAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(HashOperations));
    hash->synch = getAlignedMemory(CACHE_LINE_SIZE, hash_size * sizeof(DSMSynchStruct));
    hash->buckets = getAlignedMemory(CACHE_LINE_SIZE, hash_size * sizeof(ptr_aligned_t));
    for (i = 0; i < hash_size; i++) {
        DSMSynchStructInit(&hash->synch[i], nthreads);
        hash->buckets[i].ptr = NULL;
    }
}

inline void DSMHashThreadStateInit(DSMHash *hash, DSMHashThreadState *th_state, int hash_size, int pid) {
    int i;

    th_state->th_state = getMemory(hash_size * sizeof(DSMSynchThreadState));
    init_pool(&th_state->pool, sizeof(HashNode));
    for (i = 0; i < hash_size; i++)
        DSMSynchThreadStateInit(&hash->synch[i], &th_state->th_state[i], pid);
}

static inline int64_t hash_func(DSMHash *hash, int64_t key) {
    return key % hash->size;
}

static inline RetVal serialOperations(void *h, ArgVal dummy_arg, int pid) {
    HashOperations arg;
    int64_t key;
    int64_t value;
    DSMHash *hash = (DSMHash *)h;
    ptr_aligned_t *buckets;
    HashNode *top;

    arg = hash->announce[pid];
    key = arg.key;
    value = arg.value;
    buckets = (ptr_aligned_t *)hash->buckets;
    top = buckets[arg.bucket].ptr;
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
            buckets[arg.bucket].ptr = arg.node;
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
                top->next = cur->next;
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

inline void DSMHashInsert(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int64_t value, int pid) {
    HashOperations args;

    args.op = HT_INSERT;
    args.key = key;
    args.value = value;
    args.bucket = hash_func(hash, key);
    args.node = alloc_obj(&th_state->pool);
    hash->announce[pid] = args;
    DSMSynchApplyOp(&hash->synch[args.bucket], &th_state->th_state[args.bucket], serialOperations, (void *)hash, 0, pid);
}

inline void DSMHashSearch(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;

    args.op = HT_SEARCH;
    args.key = key;
    args.value = INT_MIN;
    args.bucket = hash_func(hash, key);
    args.node = NULL;
    hash->announce[pid] = args;
    DSMSynchApplyOp(&hash->synch[args.bucket], &th_state->th_state[args.bucket], serialOperations, (void *)hash, 0, pid);
}

inline void DSMHashDelete(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;

    args.op = HT_DELETE;
    args.key = key;
    args.value = INT_MIN;
    args.bucket = hash_func(hash, key);
    args.node = NULL;
    hash->announce[pid] = args;
    DSMSynchApplyOp(&hash->synch[args.bucket], &th_state->th_state[args.bucket], serialOperations, (void *)hash, 0, pid);
}
