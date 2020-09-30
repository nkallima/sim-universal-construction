#ifndef _HSYNCH_H_
#define _HSYNCH_H_

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif

#include <system.h>
#include <config.h>
#include <primitives.h>
#include <clh.h>
#include <fastrand.h>

typedef struct HalfHSynchNode {
    struct HalfHSynchNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
} HalfHSynchNode;

typedef struct HSynchNode {
    struct HSynchNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
    int32_t align[PAD_CACHE(sizeof(HalfHSynchNode))];
} HSynchNode;

typedef union HSynchNodePtr {
    struct HSynchNode *ptr;
    char pad[CACHE_LINE_SIZE];
} HSynchNodePtr;

typedef struct HSynchThreadState {
    HSynchNode *next_node;
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif
} HSynchThreadState;

typedef struct HSynchStruct {
    CLHLockStruct *central_lock CACHE_ALIGN;
    volatile HSynchNodePtr *Tail CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} HSynchStruct;


RetVal HSynchApplyOp(HSynchStruct *l, HSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);
void HSynchThreadStateInit(HSynchThreadState *st_thread, int pid);
void HSynchStructInit(HSynchStruct *l);
#endif
