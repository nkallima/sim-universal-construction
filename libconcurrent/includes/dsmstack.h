#ifndef _DSMSTACK_H_
#define _DSMSTACK_H_

#include <dsmsynch.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

typedef struct DSMStackStruct {
    DSMSynchStruct object_struct CACHE_ALIGN;
    volatile Node * volatile head CACHE_ALIGN;
} DSMStackStruct;


typedef struct DSMStackThreadState {
    DSMSynchThreadState th_state;
} DSMStackThreadState;


void DSMSStackInit(DSMStackStruct *stack_object_struct, uint32_t nthreads);
void DSMStackThreadStateInit(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, int pid);
void DSMStackPush(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, ArgVal arg, int pid);
void DSMStackPop(DSMStackStruct *object_struct, DSMStackThreadState *lobject_struct, int pid);

#endif
