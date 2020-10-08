#ifndef _HSTACK_H_
#define _HSTACK_H_

#include <hsynch.h>
#include <config.h>
#include <primitives.h>
#include <pool.h>
#include <queue-stack.h>

typedef struct HStackStruct {
    HSynchStruct object_struct CACHE_ALIGN;
    volatile Node *head CACHE_ALIGN;
} HStackStruct;


typedef struct HStackThreadState {
    HSynchThreadState th_state;
} HStackThreadState;

void HStackInit(HStackStruct *stack_object_struct, uint32_t nthreads);
void HStackThreadStateInit(HStackStruct *object_struct, HStackThreadState *lobject_struct, int pid);
void HStackPush(HStackStruct *object_struct, HStackThreadState *lobject_struct, ArgVal arg, int pid);
RetVal HStackPop(HStackStruct *object_struct, HStackThreadState *lobject_struct, int pid);

#endif
