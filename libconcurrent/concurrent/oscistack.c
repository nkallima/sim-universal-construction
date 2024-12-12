#include <oscistack.h>
#include <threadtools.h>

static RetVal serialPushPop(void *state, ArgVal arg, int pid);

static const int POP_OP = INT_MIN;

void OsciStackInit(OsciStackStruct *stack_object_struct, uint32_t nthreads, uint32_t fibers_per_thread) {
    OsciInit(&(stack_object_struct->object_struct), nthreads, fibers_per_thread);
    stack_object_struct->pool_node = synchGetAlignedMemory(CACHE_LINE_SIZE, stack_object_struct->object_struct.groups_of_fibers * sizeof(SynchPoolStruct));
    stack_object_struct->top = NULL;
}

void OsciStackThreadStateInit(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, int pid) {
    OsciThreadStateInit(&(lobject_struct->th_state), &object_struct->object_struct, (int)pid);
    synchInitPool(&(object_struct->pool_node[synchGetThreadId()]), sizeof(Node));
}

static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile OsciStackStruct *st = (OsciStackStruct *)state;
        volatile Node *node = st->top;

        if (st->top != NULL) {
            RetVal ret = node->val;
            st->top = st->top->next;
            synchRecycleObj(&(st->pool_node[synchGetThreadId()]), (void *)node);
            return ret;
        }

        return EMPTY_STACK;
    } else {
        OsciStackStruct *st = (OsciStackStruct *)state;
        Node *node;

        node = synchAllocObj(&(st->pool_node[synchGetThreadId()]));
        node->next = st->top;
        node->val = arg;
        st->top = node;

        return PUSH_SUCCESS;
    }
}

void OsciStackApplyPush(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, ArgVal arg, int pid) {
    OsciApplyOp(&(object_struct->object_struct), &(lobject_struct->th_state), serialPushPop, object_struct, (ArgVal)arg, pid);
}

RetVal OsciStackApplyPop(OsciStackStruct *object_struct, OsciStackThreadState *lobject_struct, int pid) {
    return OsciApplyOp(&(object_struct->object_struct), &(lobject_struct->th_state), serialPushPop, object_struct, POP_OP, pid);
}
