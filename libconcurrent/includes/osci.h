#ifndef __OSCI_H_
#define __OSCI_H_

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <uthreads.h>
#include <types.h>

typedef struct OsciFiberRec {
    volatile ArgVal arg_ret;
    volatile int32_t pid;
    volatile int16_t locked;
    volatile int16_t completed;
} OsciFiberRec;

typedef struct HalfOsciNode {
    volatile struct HalfOsciNode *next;
    volatile int32_t toggle;
    volatile int32_t door;
    volatile OsciFiberRec rec[FIBERS_PER_THREAD];
} HalfOsciNode;

typedef struct OsciNode {
    volatile struct OsciNode *next;
    volatile int32_t toggle;
    volatile int32_t door;
    volatile OsciFiberRec rec[FIBERS_PER_THREAD];
    char pad[PAD_CACHE(sizeof(HalfOsciNode))];
} OsciNode;

typedef struct OsciThreadState {
    volatile OsciNode next_node[2];
    int toggle CACHE_ALIGN;
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif
} OsciThreadState;

typedef struct OsciStruct {
    volatile ptr_aligned_t current_node[FIBERS_GROUP] CACHE_ALIGN;
    volatile OsciNode *Tail CACHE_ALIGN;
    uint32_t nthreads CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} OsciStruct;

void OsciInit(OsciStruct *l, uint32_t nthreads);
void OsciThreadStateInit(OsciThreadState *st_thread, int pid);
RetVal OsciApplyOp(OsciStruct *l, OsciThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif
