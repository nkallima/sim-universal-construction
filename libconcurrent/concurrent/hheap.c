#include <serialheap.h>
#include <hheap.h>

void HSynchHeapInit(HSynchHeapStruct *heap_struct, uint32_t type, uint32_t nthreads, uint32_t numa_nodes) {
    serialHeapInit(&heap_struct->state, type);
    HSynchStructInit(&heap_struct->heap, nthreads, numa_nodes);
}

void HSynchHeapThreadStateInit(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, int pid) {
    HSynchThreadStateInit(&heap_struct->heap, &lobject_struct->thread_state, pid);
}

void HSynchHeapInsert(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, SynchHeapElement arg, int pid) {
    HSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, arg | SYNCH_HEAP_INSERT_OP, pid);
}

SynchHeapElement HSynchHeapDeleteMin(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, int pid) {
    return HSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_DELETE_MIN_MAX_OP, pid);
}

SynchHeapElement HSynchHeapGetMin(HSynchHeapStruct *heap_struct, HSynchHeapThreadState *lobject_struct, int pid) {
    return HSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_GET_MIN_MAX_OP, pid);
}
