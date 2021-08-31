#include <ccstack.h>
#include <pool.h>

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;
static __thread SynchPoolStruct pool_node CACHE_ALIGN;

void CCStackInit(CCStackStruct *stack_object_struct, uint32_t nthreads) {
    CCSynchStructInit(&stack_object_struct->object_struct, nthreads);
    stack_object_struct->top = NULL;
    synchStoreFence();
}

void CCStackThreadStateInit(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid) {
    CCSynchThreadStateInit(&object_struct->object_struct, &lobject_struct->th_state, (int)pid);
    synchInitPool(&pool_node, sizeof(Node));
}

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile CCStackStruct *st = (CCStackStruct *)state;
        volatile Node *node = st->top;

        if (st->top != NULL) {
            RetVal ret = node->val;
            st->top = st->top->next;
            synchNonTSOFence();
            synchRecycleObj(&pool_node, (void *)node);
            return ret;
        } else
            return EMPTY_STACK;
    } else {
        CCStackStruct *st = (CCStackStruct *)state;
        Node *node;

        node = synchAllocObj(&pool_node);
        node->next = st->top;
        node->val = arg;
        st->top = node;
        synchNonTSOFence();

        return PUSH_SUCCESS;
    }
}

void CCStackPush(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, ArgVal arg, int pid) {
    CCSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)arg, pid);
}

RetVal CCStackPop(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid) {
    return CCSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)POP_OP, pid);
}
