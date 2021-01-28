#include <ccqueue.h>

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid);
inline static RetVal serialDequeue(void *state, ArgVal arg, int pid);

static __thread PoolStruct pool_node CACHE_ALIGN;

void CCQueueStructInit(CCQueueStruct *queue_object_struct, uint32_t nthreads) {
    CCSynchStructInit(&queue_object_struct->enqueue_struct, nthreads);
    CCSynchStructInit(&queue_object_struct->dequeue_struct, nthreads);
    queue_object_struct->guard.val = GUARD_VALUE;
    queue_object_struct->guard.next = null;
    queue_object_struct->first = &queue_object_struct->guard;
    queue_object_struct->last = &queue_object_struct->guard;
}

void CCQueueThreadStateInit(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, int pid) {
    CCSynchThreadStateInit(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, (int)pid);
    CCSynchThreadStateInit(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid) {
    CCQueueStruct *st = (CCQueueStruct *)state;
    Node *node;

    node = alloc_obj(&pool_node);
    node->next = null;
    node->val = arg;
    st->last->next = node;
    st->last = node;
    return -1;
}

inline static RetVal serialDequeue(void *state, ArgVal arg, int pid) {
    CCQueueStruct *st = (CCQueueStruct *)state;
    volatile Node *node, *prev;

    if (st->first->next != null){
        prev = st->first;
        st->first = st->first->next;
        node = st->first;
        if (node->val == GUARD_VALUE)
            return serialDequeue(state, arg, pid);
        recycle_obj(&pool_node, (Node *)prev);
        return node->val;
    } else {
        return -1;
    }
}

void CCQueueApplyEnqueue(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, ArgVal arg, int pid) {
    CCSynchApplyOp(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, serialEnqueue, object_struct, (ArgVal)arg, pid);
}

RetVal CCQueueApplyDequeue(CCQueueStruct *object_struct, CCQueueThreadState *lobject_struct, int pid) {
    return CCSynchApplyOp(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, serialDequeue, object_struct, (ArgVal)pid, pid);
}
