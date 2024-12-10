#include <serialheap.h>
#include <fcheap.h>

void FCHeapInit(FCHeapStruct *heap_struct, uint32_t type, uint32_t nthreads) {
    FCStructInit(&heap_struct->heap, nthreads);
    serialHeapInit(&heap_struct->state, type);
}

void FCHeapThreadStateInit(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, int pid) {
    FCThreadStateInit(&heap_struct->heap, &lobject_struct->thread_state, pid);
}

void FCHeapInsert(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, SynchHeapElement arg, int pid) {
    FCApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, arg | SYNCH_HEAP_INSERT_OP, pid);
}

SynchHeapElement FCHeapDeleteMin(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, int pid) {
    return FCApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_DELETE_MIN_MAX_OP, pid);
}

SynchHeapElement FCHeapGetMin(FCHeapStruct *heap_struct, FCHeapThreadState *lobject_struct, int pid) {
    return FCApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_GET_MIN_MAX_OP, pid);
}
