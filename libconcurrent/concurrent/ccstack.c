#include <ccstack.h>

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;
static __thread PoolStruct pool_node CACHE_ALIGN;

void CCStackInit(CCStackStruct *stack_object_struct, uint32_t nthreads) {
    CCSynchStructInit(&stack_object_struct->object_struct, nthreads);
    stack_object_struct->head = null;
    StoreFence();
}

void CCStackThreadStateInit(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid) {
    CCSynchThreadStateInit(&object_struct->object_struct, &lobject_struct->th_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile CCStackStruct *st = (CCStackStruct *)state;
        volatile Node *node = st->head;

        if (st->head != null) {
            st->head = st->head->next;
            return node->val;
        } else return -1;
    } else {
        CCStackStruct *st = (CCStackStruct *)state;
        Node *node;

        node = alloc_obj(&pool_node);
        node->next = st->head;
        node->val = arg;
        st->head = node;
 
        return 0;
    }
}

void CCStackPush(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, ArgVal arg, int pid) {
    CCSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) arg, pid);
}

RetVal CCStackPop(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid) {
    return CCSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) POP_OP, pid);
}
