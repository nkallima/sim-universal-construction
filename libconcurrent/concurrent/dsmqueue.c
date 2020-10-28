#include <dsmqueue.h>

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid);
inline static RetVal serialDequeue(void *state, ArgVal arg, int pid);

static const int GUARD = INT_MIN;
static __thread PoolStruct pool_node CACHE_ALIGN;

void DSMQueueStructInit(DSMQueueStruct *queue_object_struct, uint32_t nthreads) {
    DSMSynchStructInit(&queue_object_struct->enqueue_struct, nthreads);
    DSMSynchStructInit(&queue_object_struct->dequeue_struct, nthreads);
    queue_object_struct->guard.val = GUARD;
    queue_object_struct->guard.next = null;
    queue_object_struct->first = &queue_object_struct->guard;
    queue_object_struct->last = &queue_object_struct->guard;
}

void DSMQueueThreadStateInit(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid) {
    DSMSynchThreadStateInit(&lobject_struct->enqueue_thread_state, (int)pid);
    DSMSynchThreadStateInit(&lobject_struct->dequeue_thread_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid) {
    DSMQueueStruct *st = (DSMQueueStruct *)state;
    Node *node;

    node = alloc_obj(&pool_node);
    node->next = null;
    node->val = arg;
    st->last->next = node;
    st->last = node;
    return -1;
}

inline static RetVal serialDequeue(void *state, ArgVal arg, int pid) {
    DSMQueueStruct *st = (DSMQueueStruct *)state;
    Node *node = (Node *)st->first;

    if (st->first->next != null) {
        st->first = st->first->next;
        return node->val;
    } else {
        return -1;
    }
}

void DSMQueueApplyEnqueue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, ArgVal arg, int pid) {
    DSMSynchApplyOp(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, serialEnqueue, object_struct, (ArgVal)pid, pid);
}

RetVal DSMQueueApplyDequeue(DSMQueueStruct *object_struct, DSMQueueThreadState *lobject_struct, int pid) {
    return DSMSynchApplyOp(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, serialDequeue, object_struct, (ArgVal)pid, pid);
}
