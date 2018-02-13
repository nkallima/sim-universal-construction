#include <clhhash.h>

static inline int64_t hash_func(CLHHash *hash, int64_t key);
static inline RetVal serialOperations(void *h, ArgVal dummy_arg, int pid);

static const int HT_INSERT = 0, HT_DELETE = 1, HT_SEARCH = 2;

inline void CLHHashInit(CLHHash *hash, int hash_size, int nthreads) {
    int i;
    
    hash->size = hash_size;
    hash->announce = getAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof (HashOperations));
    hash->synch = getAlignedMemory(CACHE_LINE_SIZE, hash_size * sizeof (CLHLockStruct *));
    hash->buckets = getAlignedMemory(CACHE_LINE_SIZE, hash_size * sizeof (ptr_aligned_t));
    for(i = 0; i < hash_size; i++) {
        hash->synch[i] = CLHLockInit();  
        hash->buckets[i].ptr = null;
    }
}

inline void CLHHashThreadStateInit(CLHHash *hash, CLHHashThreadState *th_state, int hash_size, int pid) {
    init_pool(&th_state->pool, sizeof(HashNode));
}

static inline int64_t hash_func(CLHHash *hash, int64_t key) {
    return key % hash->size;
}

static inline RetVal serialOperations(void *h, ArgVal dummy_arg, int pid) {
    HashOperations arg;
    int64_t key;
    int64_t value;
    CLHHash *hash = (CLHHash *)h;
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
        
        while (cur != null && found == false) {
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
    } else if (arg.op == HT_DELETE){
        bool found = false;
        HashNode *cur = top, *prev = top;
        
        while (cur != null && found == false) {
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
    } else { //SEARCH
        bool found = false;
        HashNode *cur = top;
        
        while (cur != null && found == false) {
            if (cur->key == key) {
                found = true;
                break;
            }
            cur = cur->next;
        }
        return found;
    }
}

inline void CLHHashInsert(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int64_t value, int pid) {
    HashOperations args;
    
    args.op = HT_INSERT;
    args.key = key;
    args.value = value;
    args.bucket = hash_func(hash, key);
    args.node = alloc_obj(&th_state->pool);
    hash->announce[pid] = args;
    
    CLHLock(hash->synch[args.bucket], pid);
    serialOperations((void *)hash, 0, pid);
    CLHUnlock(hash->synch[args.bucket], pid);
}

inline void CLHHashSearch(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;
    
    args.op = HT_SEARCH;
    args.key = key;
    args.value = INT_MIN;
    args.bucket = hash_func(hash, key);
    args.node = alloc_obj(&th_state->pool);
    hash->announce[pid] = args;

    CLHLock(hash->synch[args.bucket], pid);
    serialOperations((void *)hash, 0, pid);
    CLHUnlock(hash->synch[args.bucket], pid);
}


inline void CLHHashDelete(CLHHash *hash, CLHHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;
    
    args.op = HT_DELETE;
    args.key = key;
    args.value = INT_MIN;
    args.bucket = hash_func(hash, key);
    args.node = alloc_obj(&th_state->pool);
    hash->announce[pid] = args;

    CLHLock(hash->synch[args.bucket], pid);
    serialOperations((void *)hash, 0, pid);
    CLHUnlock(hash->synch[args.bucket], pid);
}
