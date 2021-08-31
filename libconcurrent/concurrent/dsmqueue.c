#include <dsmqueue.h>
#include <pool.h>

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid);
inline static RetVal serialDequeue(void *state, ArgVal arg, int pid);

static __thread SynchPoolStruct pool_node CACHE_ALIGN;

void DSMQueueStructInit(DSMQueueStruct *queue_object_struct, uint32_t nthreads) {
    DSMSynchStructInit(&queue_object_struct->enqueue_struct, nthreads);
    DSMSynchStructInit(&queue_object_struct->dequeue_struct, nthreads);
    queue_object_struct->guard.val = GUARD_VALUE;
    queue_object_struct->guard.next = NULL;
    queue_object_struct->first = &queue_object_struct->guard;
    queue_object_struct->last = &queue_object_struct->guard;
}

void DSMQueueThreadStateInit(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid) {
    DSMSynchThreadStateInit(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, (int)pid);
    DSMSynchThreadStateInit(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, (int)pid);
    synchInitPool(&pool_node, sizeof(Node));
}

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid) {
    DSMQueueStruct *st = (DSMQueueStruct *)state;
    Node *node;

    node = synchAllocObj(&pool_node);
    node->next = NULL;
    node->val = arg;
    st->last->next = node;
    st->last = node;
    synchNonTSOFence();
    return ENQUEUE_SUCCESS;
}

inline static RetVal serialDequeue(void *state, ArgVal arg, int pid) {
    DSMQueueStruct *st = (DSMQueueStruct *)state;
    volatile Node *node, *prev;

    if (st->first->next != NULL){
        prev = st->first;
        st->first = st->first->next;
        node = st->first;
        if (node->val == GUARD_VALUE)
            return serialDequeue(state, arg, pid);
        synchNonTSOFence();
        synchRecycleObj(&pool_node, (Node *)prev);
        return node->val;
    } else {
        return EMPTY_QUEUE;
    }
}

void DSMQueueApplyEnqueue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, ArgVal arg, int pid) {
    DSMSynchApplyOp(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, serialEnqueue, object_struct, (ArgVal)arg, pid);
}

RetVal DSMQueueApplyDequeue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid) {
    return DSMSynchApplyOp(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, serialDequeue, object_struct, (ArgVal)pid, pid);
}
