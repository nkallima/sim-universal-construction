#ifndef _DSMSYNCH_H_
#define _DSMSYNCH_H_

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
