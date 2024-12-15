#include <serialheap.h>

static SynchHeapElement serialHeapDeleteMinMax(SerialHeapStruct *heap_state);
static SynchHeapElement serialHeapInsert(SerialHeapStruct *heap_state, SynchHeapElement el);
static SynchHeapElement serialHeapGetMinMax(SerialHeapStruct *heap_state);
static void serialHeapCorrectDown(SerialHeapStruct *heap_state, uint32_t level, uint32_t pos);
static void serialHeapCorrectUp(SerialHeapStruct *heap_state);
static bool serialHeapGrow(SerialHeapStruct *heap_state);

void serialHeapInit(SerialHeapStruct *heap_state, uint32_t type) {
    uint64_t i;

    heap_state->items = 0;
    heap_state->type = type;
    heap_state->levels = SYNCH_HEAP_INITIAL_LEVELS;
    heap_state->last_used_level = 0;
    heap_state->last_used_level_pos = 0;
    heap_state->last_used_level_size = 1;
    heap_state->bulk = synchGetAlignedMemory(CACHE_LINE_SIZE, SYNCH_HEAP_INITIAL_SIZE * sizeof(SynchHeapElement));
    for (i = 0; i < SYNCH_HEAP_INITIAL_SIZE; i++)
        heap_state->bulk[i] = SYNCH_HEAP_EMPTY_NODE;
    for (i = 0; i < SYNCH_HEAP_MAX_LEVELS; i++)
        heap_state->heap_arrays[i] = &heap_state->bulk[(1ULL << i) - 1];
}

static void serialHeapCorrectDown(SerialHeapStruct *heap_state, uint32_t level, uint32_t pos) {
    while (level > 0) {
        uint32_t pos_div_2 = pos / 2;
        if (heap_state->type == SYNCH_HEAP_TYPE_MIN) {
            if (heap_state->heap_arrays[level][pos] < heap_state->heap_arrays[level - 1][pos_div_2]) {
                SynchHeapElement tmp = heap_state->heap_arrays[level - 1][pos_div_2];
                heap_state->heap_arrays[level - 1][pos_div_2] = heap_state->heap_arrays[level][pos];
                heap_state->heap_arrays[level][pos] = tmp;
            } else {
                break;
            }
        } else {
            if (heap_state->heap_arrays[level][pos] > heap_state->heap_arrays[level - 1][pos_div_2]) {
                SynchHeapElement tmp = heap_state->heap_arrays[level - 1][pos_div_2];
                heap_state->heap_arrays[level - 1][pos_div_2] = heap_state->heap_arrays[level][pos];
                heap_state->heap_arrays[level][pos] = tmp;
            } else {
                break;
            }
        }
        level--;
        pos = pos_div_2;
    }
}

static void serialHeapCorrectUp(SerialHeapStruct *heap_state) {
    uint32_t level = 0;
    uint32_t pos = 0;

    while (level < heap_state->last_used_level) {
        uint32_t pos_left = 2 * pos;
        uint32_t pos_right = 2 * pos + 1;

        if (heap_state->type == SYNCH_HEAP_TYPE_MIN) {
            if (heap_state->heap_arrays[level][pos] > heap_state->heap_arrays[level + 1][pos_left] || heap_state->heap_arrays[level][pos] > heap_state->heap_arrays[level + 1][pos_right]) {
                if (heap_state->heap_arrays[level + 1][pos_left] > heap_state->heap_arrays[level + 1][pos_right]) {  // Go to the right
                    SynchHeapElement tmp = heap_state->heap_arrays[level + 1][pos_right];
                    heap_state->heap_arrays[level + 1][pos_right] = heap_state->heap_arrays[level][pos];
                    heap_state->heap_arrays[level][pos] = tmp;
                    pos = pos_right;
                } else {  // Go to the left
                    SynchHeapElement tmp = heap_state->heap_arrays[level + 1][pos_left];
                    heap_state->heap_arrays[level + 1][pos_left] = heap_state->heap_arrays[level][pos];
                    heap_state->heap_arrays[level][pos] = tmp;
                    pos = pos_left;
                }
            } else {
                break;
            }
        } else {
            if (heap_state->heap_arrays[level][pos] < heap_state->heap_arrays[level + 1][pos_left] || heap_state->heap_arrays[level][pos] < heap_state->heap_arrays[level + 1][pos_right]) {
                if (heap_state->heap_arrays[level + 1][pos_left] < heap_state->heap_arrays[level + 1][pos_right]) {  // Go to the right
                    SynchHeapElement tmp = heap_state->heap_arrays[level + 1][pos_right];
                    heap_state->heap_arrays[level + 1][pos_right] = heap_state->heap_arrays[level][pos];
                    heap_state->heap_arrays[level][pos] = tmp;
                    pos = pos_right;
                } else {  // Go to the left
                    SynchHeapElement tmp = heap_state->heap_arrays[level + 1][pos_left];
                    heap_state->heap_arrays[level + 1][pos_left] = heap_state->heap_arrays[level][pos];
                    heap_state->heap_arrays[level][pos] = tmp;
                    pos = pos_left;
                }
            } else {
                break;
            }
        }
        level++;
    }
}

static SynchHeapElement serialHeapDeleteMinMax(SerialHeapStruct *heap_state) {
    SynchHeapElement ret = serialHeapGetMinMax(heap_state);

    if (ret != SYNCH_HEAP_EMPTY) {
        if (heap_state->last_used_level_pos > 0) {
            heap_state->last_used_level_pos -= 1;
            heap_state->heap_arrays[0][0] = heap_state->heap_arrays[heap_state->last_used_level][heap_state->last_used_level_pos];
            if (heap_state->last_used_level_pos == 0) {
                heap_state->last_used_level -= 1;
                heap_state->last_used_level_size = SYNCH_HEAP_SIZE_OF_LEVEL(heap_state->last_used_level);
                heap_state->last_used_level_pos = heap_state->last_used_level_size - 1;
            }
            serialHeapCorrectUp(heap_state);
        } else if (heap_state->last_used_level > 0) {
            heap_state->last_used_level -= 1;
            heap_state->last_used_level_size = SYNCH_HEAP_SIZE_OF_LEVEL(heap_state->last_used_level);
            heap_state->last_used_level_pos = heap_state->last_used_level_size - 1;
            heap_state->heap_arrays[0][0] = heap_state->heap_arrays[heap_state->last_used_level][heap_state->last_used_level_pos];
            serialHeapCorrectUp(heap_state);
        } else if (heap_state->last_used_level == 0) {
            heap_state->last_used_level_pos = 0;
        }
        heap_state->items -= 1;
    }


    return ret;
}

static bool serialHeapGrow(SerialHeapStruct *heap_state) {
    SynchHeapElement *expanded_bulk = NULL;
    uint64_t i;

    if (heap_state->levels == SYNCH_HEAP_MAX_LEVELS)
        return false;
    expanded_bulk = synchGetAlignedMemory(CACHE_LINE_SIZE, SYNCH_HEAP_SIZE_OF_LEVEL(heap_state->levels + 1) * sizeof(SynchHeapElement));
    if (expanded_bulk == NULL)
        return false;
    memcpy(expanded_bulk, heap_state->bulk, SYNCH_HEAP_SIZE_OF_LEVEL(heap_state->levels) * sizeof(SynchHeapElement));
    heap_state->bulk = expanded_bulk;
    heap_state->levels += 1;
    for (i = SYNCH_HEAP_SIZE_OF_LEVEL(heap_state->levels - 1); i < SYNCH_HEAP_SIZE_OF_LEVEL(heap_state->levels); i++)
        heap_state->bulk[i] = SYNCH_HEAP_EMPTY_NODE;
    for (i = 0; i < heap_state->levels; i++)
        heap_state->heap_arrays[i] = &heap_state->bulk[(1ULL << i) - 1];

    return true;
}

static SynchHeapElement serialHeapInsert(SerialHeapStruct *heap_state, SynchHeapElement el) {
    // Check if there is enough space inside the last level
    if (heap_state->last_used_level_pos < heap_state->last_used_level_size) {
        heap_state->heap_arrays[heap_state->last_used_level][heap_state->last_used_level_pos] = el;
        heap_state->last_used_level_pos += 1;
        serialHeapCorrectDown(heap_state, heap_state->last_used_level, heap_state->last_used_level_pos - 1);
        heap_state->items += 1;
        return SYNCH_HEAP_INSERT_SUCCESS;
    } else if (heap_state->last_used_level < heap_state->levels - 1) {  // There is free space, but not in the last used level, There is no need to allocate a new level
        heap_state->last_used_level_size *= 2;
        heap_state->last_used_level += 1;
        heap_state->last_used_level_pos = 1;
        heap_state->heap_arrays[heap_state->last_used_level][0] = el;
        serialHeapCorrectDown(heap_state, heap_state->last_used_level, heap_state->last_used_level_pos - 1);
        heap_state->items += 1;
        return SYNCH_HEAP_INSERT_SUCCESS;
    } else {  // out of space, we need to allocate more levels

        if(serialHeapGrow(heap_state)) {
            fprintf(stderr, "DEBUG: Heap levels: %d\n", heap_state->levels);
#ifdef DEBUG
            fprintf(stderr, "DEBUG: Heap space is full! Successful Heap expansion\n");
#endif
            return serialHeapInsert(heap_state, el);
        } else {
#ifdef DEBUG
            fprintf(stderr, "DEBUG: Heap space is full! Unsuccessful Heap expansion\n");
#endif
            return SYNCH_HEAP_INSERT_FAIL;
        }

    }
}

static SynchHeapElement serialHeapGetMinMax(SerialHeapStruct *heap_state) {
    if (heap_state->last_used_level != 0 && heap_state->last_used_level_pos != 0) return heap_state->heap_arrays[0][0];
    return SYNCH_HEAP_EMPTY;
}

#ifdef DEBUG
static void serialDisplay(SerialHeapStruct *heap_state) {
    int i, j;

    for (i = 0; i < heap_state->last_used_level; i++) {
        for (j = 0; j < SYNCH_HEAP_SIZE_OF_LEVEL(i); j++) {
            if (heap_state->heap_arrays[i][j] >= 0) printf("  %ld  ", heap_state->heap_arrays[i][j]);
        }
        printf("\n");
    }
    // Print the last level of the heap
    for (j = 0; j < heap_state->last_used_level_pos; j++) {
        if (heap_state->heap_arrays[heap_state->last_used_level][j] >= 0) printf("  %ld  ", heap_state->heap_arrays[heap_state->last_used_level][j]);
    }
    printf("\n");
}
#endif

bool serialHeapClearAndValidation(SerialHeapStruct *heap_state) {
    SynchHeapElement elmnt, prev;
    uint64_t i = 0;
#ifdef DEBUG
    //serialDisplay(heap_state);
#endif
    while ((elmnt = serialHeapDeleteMinMax(heap_state)) != SYNCH_HEAP_EMPTY) {
        if (i > 0 && heap_state->type == SYNCH_HEAP_TYPE_MIN && prev > elmnt) 
            return false;
        if (i > 0 && heap_state->type == SYNCH_HEAP_TYPE_MAX && prev < elmnt) 
            return false;
        prev = elmnt;
        i++;
    }

    return true;
}

RetVal serialHeapApplyOperation(void *state, ArgVal arg, int pid) {
    SynchHeapElement ret = SYNCH_HEAP_EMPTY;
    uint64_t op = arg & SYNCH_HEAP_OP_MASK;
    uint64_t val = arg & SYNCH_HEAP_VAL_MASK;

    switch (op) {
    case SYNCH_HEAP_INSERT_OP:
        ret = serialHeapInsert(state, val);
        break;
    case SYNCH_HEAP_DELETE_MIN_MAX_OP:
        ret = serialHeapDeleteMinMax(state);
        break;
    case SYNCH_HEAP_GET_MIN_MAX_OP:
        ret = serialHeapGetMinMax(state);
        break;
    default:
        fprintf(stderr, "ERROR: Invalid heap operation\n");
        break;
    }
    return ret;
}