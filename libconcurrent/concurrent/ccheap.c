#include <serialheap.h>
#include <ccheap.h>

void CCHeapInit(CCHeapStruct *heap_struct, uint32_t type, uint32_t nthreads) {
    serialHeapInit(&heap_struct->state, type);
    CCSynchStructInit(&heap_struct->heap, nthreads);
}

void CCHeapThreadStateInit(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, int pid) {
    CCSynchThreadStateInit(&heap_struct->heap, &lobject_struct->thread_state, pid);
}

void CCHeapInsert(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, SynchHeapElement arg, int pid) {
    CCSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, arg | SYNCH_HEAP_INSERT_OP, pid);
}

SynchHeapElement CCHeapDeleteMin(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, int pid) {
    return CCSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_DELETE_MIN_MAX_OP, pid);
}

SynchHeapElement CCHeapGetMin(CCHeapStruct *heap_struct, CCHeapThreadState *lobject_struct, int pid) {
    return CCSynchApplyOp(&heap_struct->heap, &lobject_struct->thread_state, serialHeapApplyOperation, &heap_struct->state, SYNCH_HEAP_GET_MIN_MAX_OP, pid);
}
