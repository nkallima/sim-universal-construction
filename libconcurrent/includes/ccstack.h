#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include <ccsynch.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

typedef struct CCStackStruct {
    CCSynchStruct object_struct CACHE_ALIGN;
    volatile Node *volatile head CACHE_ALIGN;
} CCStackStruct;

typedef struct CCStackThreadState {
    CCSynchThreadState th_state;
} CCStackThreadState;

void CCStackInit(CCStackStruct *stack_object_struct, uint32_t nthreads);
void CCStackThreadStateInit(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid);
void CCStackPush(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, ArgVal arg, int pid);
RetVal CCStackPop(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid);

#endif
