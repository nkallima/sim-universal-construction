#ifndef _DSMSYNCH_H_
#define _DSMSYNCH_H_

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif
#include <config.h>

#include <primitives.h>

typedef struct HalfDSMSynchNode {
    struct DSMSynchNode *next;
    ArgVal arg;
    RetVal ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
} HalfDSMSynchNode;

typedef struct DSMSynchNode {
    struct DSMSynchNode *next;
    ArgVal arg;
    RetVal ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
    char align[PAD_CACHE(sizeof(HalfDSMSynchNode))];
} DSMSynchNode;

typedef struct DSMSynchThreadState {
    DSMSynchNode *MyNodes[2];
    int toggle;
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif
} DSMSynchThreadState;

typedef struct DSMSynchStruct {
    volatile DSMSynchNode *Tail CACHE_ALIGN;
    uint32_t nthreads CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} DSMSynchStruct;


void DSMSynchStructInit(DSMSynchStruct *l, uint32_t ntreads);
void DSMSynchThreadStateInit(DSMSynchThreadState *st_thread, int pid);
RetVal DSMSynchApplyOp(DSMSynchStruct *l, DSMSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif
