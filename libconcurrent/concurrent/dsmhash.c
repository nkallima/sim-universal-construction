#include <dsmhash.h>
#include <limits.h>

static int64_t hash_func(DSMHash *hash, int64_t key);
static RetVal serialOperations(void *h, ArgVal dummy_arg, int pid);

static const int HT_INSERT = 0, HT_DELETE = 1, HT_SEARCH = 2;

void DSMHashInit(DSMHash *hash, int num_cells, int nthreads) {
    int i;

    hash->size = num_cells;
    hash->announce = synchGetAlignedMemory(CACHE_LINE_SIZE, nthreads * sizeof(HashOperations));
    hash->synch = synchGetAlignedMemory(CACHE_LINE_SIZE, num_cells * sizeof(DSMSynchStruct));
    hash->cells = synchGetAlignedMemory(CACHE_LINE_SIZE, num_cells * sizeof(ptr_aligned_t));
    for (i = 0; i < num_cells; i++) {
        DSMSynchStructInit(&hash->synch[i], nthreads);
        hash->cells[i].ptr = NULL;
    }
}

void DSMHashThreadStateInit(DSMHash *hash, DSMHashThreadState *th_state, int num_cells, int pid) {
    int i;

    th_state->th_state = synchGetMemory(num_cells * sizeof(DSMSynchThreadState));
    synchInitPool(&th_state->pool, sizeof(HashNode));
    for (i = 0; i < num_cells; i++)
        DSMSynchThreadStateInit(&hash->synch[i], &th_state->th_state[i], pid);
}

static int64_t hash_func(DSMHash *hash, int64_t key) {
    return key % hash->size;
}

static RetVal serialOperations(void *h, ArgVal dummy_arg, int pid) {
    HashOperations arg;
    int64_t key;
    int64_t value;
    DSMHash *hash = (DSMHash *)h;
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

bool DSMHashInsert(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int64_t value, int pid) {
    HashOperations args;

    args.op = HT_INSERT;
    args.key = key;
    args.value = value;
    args.cell = hash_func(hash, key);
    args.node = synchAllocObj(&th_state->pool);
    hash->announce[pid] = args;
    return DSMSynchApplyOp(&hash->synch[args.cell], &th_state->th_state[args.cell], serialOperations, (void *)hash, 0, pid);
}

RetVal DSMHashSearch(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;
    RetVal ret;

    args.op = HT_SEARCH;
    args.key = key;
    args.value = INT_MIN;
    args.cell = hash_func(hash, key);
    args.node = NULL;
    hash->announce[pid] = args;
    ret = DSMSynchApplyOp(&hash->synch[args.cell], &th_state->th_state[args.cell], serialOperations, (void *)hash, 0, pid);

    return ret;
}

void DSMHashDelete(DSMHash *hash, DSMHashThreadState *th_state, int64_t key, int pid) {
    HashOperations args;

    args.op = HT_DELETE;
    args.key = key;
    args.value = INT_MIN;
    args.cell = hash_func(hash, key);
    args.node = NULL;
    hash->announce[pid] = args;
    DSMSynchApplyOp(&hash->synch[args.cell], &th_state->th_state[args.cell], serialOperations, (void *)hash, 0, pid);
}
