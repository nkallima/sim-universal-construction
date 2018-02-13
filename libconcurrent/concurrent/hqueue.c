#include <hqueue.h>

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid);
inline static RetVal serialDequeue(void *state, ArgVal arg, int pid);

static const int GUARD = INT_MIN;
static __thread PoolStruct pool_node CACHE_ALIGN;

void HQueueInit(HQueueStruct *queue_object_struct) {
    HSynchStructInit(&queue_object_struct->enqueue_struct);
    HSynchStructInit(&queue_object_struct->dequeue_struct);
    queue_object_struct->guard.val = GUARD;
    queue_object_struct->guard.next = null;
    queue_object_struct->first = &queue_object_struct->guard;
    queue_object_struct->last = &queue_object_struct->guard;
}

void HQueueThreadStateInit(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, int pid) {
    HSynchThreadStateInit(&lobject_struct->enqueue_thread_state, (int)pid);
    HSynchThreadStateInit(&lobject_struct->dequeue_thread_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}


inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid) {
    HQueueStruct *st = (HQueueStruct *)state;
    Node *node;
    
    node = alloc_obj(&pool_node);
    node->next = null;
    node->val = arg;
    st->last->next = node;
    st->last = node;
    return -1;
}

inline static RetVal serialDequeue(void *state, ArgVal arg, int pid) {
    HQueueStruct *st = (HQueueStruct *)state;
    Node *node = (Node *)st->first;
    
    if (st->first->next != null){
        st->first = st->first->next;
        return node->val;
    } else {
        return -1;
    }
}

void HQueueApplyEnqueue(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, ArgVal arg, int pid) {
    HSynchApplyOp(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, serialEnqueue, object_struct, (ArgVal) pid, pid);
}

RetVal HQueueApplyDequeue(HQueueStruct *object_struct, HQueueThreadState *lobject_struct, int pid) {
    return HSynchApplyOp(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, serialDequeue, object_struct, (ArgVal) pid, pid);
}
