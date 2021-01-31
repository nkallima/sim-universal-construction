#include <dsmstack.h>

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;
static __thread PoolStruct pool_node CACHE_ALIGN;

void DSMSStackInit(DSMStackStruct *stack_object_struct, uint32_t nthreads) {
    DSMSynchStructInit(&stack_object_struct->object_struct, nthreads);
    stack_object_struct->top = null;
    StoreFence();
}

void DSMStackThreadStateInit(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, int pid) {
    DSMSynchThreadStateInit(&object_struct->object_struct, &lobject_struct->th_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile DSMStackStruct *st = (DSMStackStruct *)state;
        volatile Node *node = st->top;

        if (st->top != null) {
            RetVal ret = node->val;
            st->top = st->top->next;
            NonTSOFence();
            recycle_obj(&pool_node, (void *)node);
            return ret;
        } else
            return EMPTY_STACK;
    } else {
        DSMStackStruct *st = (DSMStackStruct *)state;
        Node *node;

        node = alloc_obj(&pool_node);
        node->next = st->top;
        node->val = arg;
        st->top = node;
        NonTSOFence();

        return PUSH_SUCCESS;
    }
}

void DSMStackPush(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, ArgVal arg, int pid) {
    DSMSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)arg, pid);
}

RetVal DSMStackPop(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, int pid) {
    return DSMSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)POP_OP, pid);
}
