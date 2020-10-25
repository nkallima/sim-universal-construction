#include <dsmstack.h>

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;
static __thread PoolStruct pool_node CACHE_ALIGN;

void DSMSStackInit(DSMStackStruct *stack_object_struct, uint32_t nthreads) {
    DSMSynchStructInit(&stack_object_struct->object_struct, nthreads);
    stack_object_struct->head = null;
}

void DSMStackThreadStateInit(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, int pid) {
    DSMSynchThreadStateInit(&object_struct->object_struct, &lobject_struct->th_state, (int)pid);
    init_pool(&pool_node, sizeof(Node));
}


inline static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile DSMStackStruct *st = (DSMStackStruct *)state;
        volatile Node *node = st->head;

        if (st->head != null) {
            st->head = st->head->next;
            return node->val;
        }
        return -1;
    } else {
        DSMStackStruct *st = (DSMStackStruct *)state;
        Node *node;

        node = alloc_obj(&pool_node);
        node->next = st->head;
        node->val = arg;
        st->head = node;
 
        return 0;
    }
}

void DSMStackPush(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, ArgVal arg, int pid) {
    DSMSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) arg, pid);
}

void DSMStackPop(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, int pid) {
    DSMSynchApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) POP_OP, pid);
}
