#include <oscistack.h>

inline static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;

void OsciStackInit(OsciStackStruct *stack_object_struct, uint32_t nthreads, uint32_t fibers_per_thread) {
    OsciInit(&(stack_object_struct->object_struct), nthreads, fibers_per_thread);
    stack_object_struct->pool_node = getAlignedMemory(CACHE_LINE_SIZE, stack_object_struct->object_struct.groups_of_fibers * sizeof(PoolStruct));
    stack_object_struct->head = null;
}

void OsciStackThreadStateInit(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, int pid) {
    OsciThreadStateInit(&(lobject_struct->th_state), &object_struct->object_struct, (int)pid);
    init_pool(&(object_struct->pool_node[getThreadId()]), sizeof(Node));
}


inline static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile OsciStackStruct *st = (OsciStackStruct *)state;
        volatile Node *node = st->head;

        if (st->head != null) {
            st->head = st->head->next;
            return node->val;
        }

        return -1;
    } else {
        OsciStackStruct *st = (OsciStackStruct *)state;
        Node *node;

        node = alloc_obj(&(st->pool_node[getThreadId()]));
        node->next = st->head;
        node->val = arg;
        st->head = node;
 
        return 0;
    }
}

void OsciStackApplyPush(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, ArgVal arg, int pid) {
    OsciApplyOp(&(object_struct->object_struct), &(lobject_struct->th_state), serialPushPop, object_struct, (ArgVal) arg, pid);
}

void OsciStackApplyPop(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, int pid) {
    OsciApplyOp(&(object_struct->object_struct), &(lobject_struct->th_state), serialPushPop, object_struct, POP_OP, pid);
}
