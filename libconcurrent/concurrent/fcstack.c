#include <fcstack.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

static const int POP_OP = INT_MIN;
static __thread SynchPoolStruct pool_node CACHE_ALIGN;

void FCStackInit(FCStackStruct *stack_object_struct, uint32_t nthreads) {
    FCStructInit(&stack_object_struct->object_struct, nthreads);
    stack_object_struct->top = NULL;
    synchStoreFence();
}

void FCStackThreadStateInit(FCStackStruct *object_struct, FCStackThreadState *lobject_struct, int pid) {
    FCThreadStateInit(&object_struct->object_struct, &lobject_struct->th_state, (int)pid);
    synchInitPool(&pool_node, sizeof(Node));
}

static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        volatile FCStackStruct *st = (FCStackStruct *)state;
        volatile Node *node = st->top;

        if (st->top != NULL) {
            st->top = st->top->next;
            return node->val;
        } else return -1;
    } else {
        FCStackStruct *st = (FCStackStruct *)state;
        Node *node;

        node = synchAllocObj(&pool_node);
        node->next = st->top;
        node->val = arg;
        st->top = node;
 
        return 0;
    }
}

void FCStackPush(FCStackStruct *object_struct, FCStackThreadState *lobject_struct, ArgVal arg, int pid) {
    FCApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) arg, pid);
}

RetVal FCStackPop(FCStackStruct *object_struct, FCStackThreadState *lobject_struct, int pid) {
    return FCApplyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) POP_OP, pid);
}
