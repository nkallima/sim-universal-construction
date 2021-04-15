#include <osciqueue.h>
#include <uthreads.h>

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid);
inline static RetVal serialDequeue(void *state, ArgVal arg, int pid);


void OsciQueueInit(OsciQueueStruct *queue_object_struct, uint32_t nthreads, uint32_t fibers_per_thread) {
    OsciInit(&(queue_object_struct->enqueue_struct), nthreads, fibers_per_thread);
    OsciInit(&queue_object_struct->dequeue_struct, nthreads, fibers_per_thread);
    queue_object_struct->pool_node = getAlignedMemory(CACHE_LINE_SIZE, queue_object_struct->enqueue_struct.groups_of_fibers * sizeof(PoolStruct));
    queue_object_struct->guard.val = GUARD_VALUE;
    queue_object_struct->guard.next = NULL;
    queue_object_struct->first = &queue_object_struct->guard;
    queue_object_struct->last = &queue_object_struct->guard;
}

void OsciQueueThreadStateInit(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, int pid) {
    OsciThreadStateInit(&lobject_struct->enqueue_thread_state, &object_struct->enqueue_struct, (int)pid);
    OsciThreadStateInit(&lobject_struct->dequeue_thread_state, &object_struct->dequeue_struct, (int)pid);
    init_pool(&(object_struct->pool_node[getThreadId()]), sizeof(Node));
}

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid) {
    OsciQueueStruct *st = (OsciQueueStruct *)state;
    Node *node;

    node = alloc_obj(&(st->pool_node[getThreadId()]));
    node->next = NULL;
    node->val = arg;
    st->last->next = node;
    st->last = node;
    return ENQUEUE_SUCCESS;
}

inline static RetVal serialDequeue(void *state, ArgVal arg, int pid) {
    OsciQueueStruct *st = (OsciQueueStruct *)state;
    volatile Node *node, *prev;

    if (st->first->next != NULL){
        prev = st->first;
        st->first = st->first->next;
        node = st->first;
        if (node->val == GUARD_VALUE)
            return serialDequeue(state, arg, pid);
        recycle_obj(&(st->pool_node[getThreadId()]), (Node *)prev);
        return node->val;
    } else {
        return EMPTY_QUEUE;
    }
}

void OsciQueueApplyEnqueue(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, ArgVal arg, int pid) {
    OsciApplyOp(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, serialEnqueue, object_struct, (ArgVal)arg, pid);
}

RetVal OsciQueueApplyDequeue(OsciQueueStruct *object_struct, OsciQueueThreadState *lobject_struct, int pid) {
    return OsciApplyOp(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, serialDequeue, object_struct, (ArgVal)pid, pid);
}
