#ifndef _CCSYNCH_H_
#define _CCSYNCH_H_

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif

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
    int32_t align[PAD_CACHE(sizeof(HalfCCSynchNode))];
} CCSynchNode;

typedef struct CCSynchThreadState {
    CCSynchNode *next;
    int toggle;
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif
} CCSynchThreadState;

typedef struct CCSynchStruct {
    volatile CCSynchNode *Tail CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
    volatile int combiner_counter[N_THREADS];
#endif
} CCSynchStruct;

void CCSynchThreadStateInit(CCSynchThreadState *st_thread, int pid);
RetVal CCSynchApplyOp(CCSynchStruct *l, CCSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);
void CCSynchStructInit(CCSynchStruct *l);

#endif
