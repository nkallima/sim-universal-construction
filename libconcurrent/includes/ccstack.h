#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include <ccsynch.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

typedef struct StackCCSynchStruct {
    CCSynchStruct object_struct CACHE_ALIGN;
    volatile Node * volatile head CACHE_ALIGN;
} StackCCSynchStruct;


typedef struct CCStackThreadState {
    CCSynchThreadState th_state;
} CCStackThreadState;


void CCStackInit(StackCCSynchStruct *stack_object_struct);
void CCStackThreadStateInit(StackCCSynchStruct *object_struct, CCStackThreadState *lobject_struct, int pid);
void CCStackPush(StackCCSynchStruct *object_struct, CCStackThreadState *lobject_struct, ArgVal arg, int pid);
RetVal CCStackPop(StackCCSynchStruct *object_struct, CCStackThreadState *lobject_struct, int pid);

#endif
