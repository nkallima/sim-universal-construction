#ifndef _CCSYNCH_H_
#define _CCSYNCH_H_

#include <config.h>
#include <primitives.h>
#include <fastrand.h>

typedef struct HalfCCSynchNode {
    struct HalfCCSynchNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
} HalfCCSynchNode;

typedef struct CCSynchNode {
    struct CCSynchNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
    char align[PAD_CACHE(sizeof(HalfCCSynchNode))];
} CCSynchNode;

typedef struct CCSynchThreadState {
    CCSynchNode *next;
    int toggle;
} CCSynchThreadState;

typedef struct CCSynchStruct {
    volatile CCSynchNode *Tail CACHE_ALIGN;
    CCSynchNode *nodes CACHE_ALIGN;
    uint32_t nthreads;
#ifdef DEBUG
    int *combiner_counter;
    volatile uint64_t counter CACHE_ALIGN;
    volatile int rounds;
#endif
} CCSynchStruct;

void CCSynchStructInit(CCSynchStruct *l, uint32_t nthreads);
void CCSynchThreadStateInit(CCSynchStruct *l, CCSynchThreadState *st_thread, int pid);
RetVal CCSynchApplyOp(CCSynchStruct *l, CCSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif
