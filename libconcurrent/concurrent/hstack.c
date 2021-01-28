#include <hstack.h>

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;
static __thread PoolStruct pool_node CACHE_ALIGN;

void HStackInit(HStackStruct *stack_object_struct, uint32_t nthreads, uint32_t numa_nodes) {
    HSynchStructInit(&stack_object_struct->object_struct, nthreads, numa_nodes);
    stack_object_struct->top = null;
}

void HStackThreadStateInit(HStackStruct *object_struct, HStackThreadState *lobject_struct, int pid) {
    HSynchThreadStateInit(&object_struct->object_struct, &lobject_struct->th_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}

inline RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile HStackStruct *st = (HStackStruct *)state;
        volatile Node *node = st->top;

        if (st->top == null) {
            return -1;
        } else {
            RetVal ret = node->val;

            st->top = st->top->next;
            recycle_obj(&pool_node, (void *)node);

            return ret;
        }
    } else {
        HStackStruct *st = (HStackStruct *)state;
        Node *node;

        node = alloc_obj(&pool_node);
        node->next = st->top;
        node->val = arg;
        st->top = node;

        return 0;
    }
}

void HStackPush(HStackStruct *object_struct, HStackThreadState *lobject_struct, ArgVal arg, int pid) {
    HSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)arg, pid);
}

RetVal HStackPop(HStackStruct *object_struct, HStackThreadState *lobject_struct, int pid) {
    return HSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)POP_OP, pid);
}
