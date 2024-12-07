#include <serialheap.h>
#include <dsmheap.h>

void DSMHeapInit(DSMHeapStruct *heap_struct, uint32_t type, uint32_t nthreads) {
    serialHeapInit(&heap_struct->state, type);
    DSMSynchStructInit(&heap_struct->heap, nthreads);
}

void DSMHeapThreadStateInit(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, int pid) {
    DSMSynchThreadStateInit(&heap_struct->heap, &lobject_struct->thread_state, pid);
}

void DSMHeapInsert(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, SynchHeapElement arg, int pid) {
    DSMSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, arg | SYNCH_HEAP_INSERT_OP, pid);
}

SynchHeapElement DSMHeapDeleteMin(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, int pid) {
    return DSMSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_DELETE_MIN_MAX_OP, pid);
}

SynchHeapElement DSMHeapGetMin(DSMHeapStruct *heap_struct, DSMHeapThreadState *lobject_struct, int pid) {
    return DSMSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_GET_MIN_MAX_OP, pid);
}
