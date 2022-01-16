#include <fcqueue.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid);
inline static RetVal serialDequeue(void *state, ArgVal arg, int pid);

static const int GUARD = INT_MIN;
static __thread SynchPoolStruct pool_node CACHE_ALIGN;

void FCQueueStructInit(FCQueueStruct *queue_object_struct, uint32_t nthreads) {
    FCStructInit(&queue_object_struct->enqueue_struct, nthreads);
    FCStructInit(&queue_object_struct->dequeue_struct, nthreads);
    queue_object_struct->guard.val = GUARD;
    queue_object_struct->guard.next = NULL;
    queue_object_struct->first = &queue_object_struct->guard;
    queue_object_struct->last = &queue_object_struct->guard;
}

void FCQueueThreadStateInit(FCQueueStruct *object_struct, FCQueueThreadState *lobject_struct, int pid) {
    FCThreadStateInit(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, (int)pid);
    FCThreadStateInit(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, (int)pid);
    synchInitPool(&pool_node, sizeof(Node));
}

inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid) {
    FCQueueStruct *st = (FCQueueStruct *)state;
    Node *node;
    
    node = synchAllocObj(&pool_node);
    node->next = NULL;
    node->val = arg;
    st->last->next = node;
    st->last = node;
    return -1;
}

inline static RetVal serialDequeue(void *state, ArgVal arg, int pid) {
    FCQueueStruct *st = (FCQueueStruct *)state;
    Node *node = (Node *)st->first;
    
    if (st->first->next != NULL){
        st->first = st->first->next;
        return node->val;
    } else {
        return -1;
    }
}

void FCQueueApplyEnqueue(FCQueueStruct *object_struct, FCQueueThreadState *lobject_struct, ArgVal arg, int pid) {
    FCApplyOp(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, serialEnqueue, object_struct, (ArgVal) pid, pid);
}

RetVal FCQueueApplyDequeue(FCQueueStruct *object_struct, FCQueueThreadState *lobject_struct, int pid) {
     return FCApplyOp(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, serialDequeue, object_struct, (ArgVal) pid, pid);
}
