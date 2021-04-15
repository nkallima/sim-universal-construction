#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include <config.h>
#include <queue-stack.h>
#include <ccsynch.h>

typedef struct CCStackStruct {
    CCSynchStruct object_struct CACHE_ALIGN;
    volatile Node *volatile top CACHE_ALIGN;
} CCStackStruct;

typedef struct CCStackThreadState {
    CCSynchThreadState th_state;
} CCStackThreadState;

void CCStackInit(CCStackStruct *stack_object_struct, uint32_t nthreads);
void CCStackThreadStateInit(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid);
void CCStackPush(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, ArgVal arg, int pid);
RetVal CCStackPop(CCStackStruct *object_struct, CCStackThreadState *lobject_struct, int pid);

#endif
