#include <hstack.h>

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;
static __thread PoolStruct pool_node CACHE_ALIGN;

void HStackInit(HStackStruct *stack_object_struct, uint32_t nthreads, uint32_t numa_nodes) {
    HSynchStructInit(&stack_object_struct->object_struct, nthreads, numa_nodes);
    stack_object_struct->head = null;
}

void HStackThreadStateInit(HStackStruct *object_struct, HStackThreadState *lobject_struct, int pid) {
    HSynchThreadStateInit(&object_struct->object_struct, &lobject_struct->th_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}

inline RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        HStackStruct *st = (HStackStruct *)state;
        Node *node;

        node = alloc_obj(&pool_node);
        node->next = st->head;
        node->val = arg;
        st->head = node;

        return 0;
    } else {
        volatile HStackStruct *st = (HStackStruct *)state;
        volatile Node *node = st->head;

        if (st->head == null) {
            return -1;
        } else {
            st->head = st->head->next;
            return node->val;
        }
        return 0;
    }
}

void HStackPush(HStackStruct *object_struct, HStackThreadState *lobject_struct, ArgVal arg, int pid) {
    HSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)arg, pid);
}

RetVal HStackPop(HStackStruct *object_struct, HStackThreadState *lobject_struct, int pid) {
    return HSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal)POP_OP, pid);
}
