#ifndef __OSCI_H_
#define __OSCI_H_

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

typedef struct OsciNode {
    volatile struct OsciNode *next;
    volatile int32_t toggle;
    volatile int32_t door;
    volatile OsciFiberRec *rec;
} OsciNode;

typedef struct OsciThreadState {
    volatile OsciNode next_node[2];
    int toggle;
} OsciThreadState;

typedef struct OsciStruct {
    volatile OsciNode *Tail CACHE_ALIGN;
    uint32_t nthreads CACHE_ALIGN;
    uint32_t fibers_per_thread;
    uint32_t groups_of_fibers;
    ptr_aligned_t *current_node;
#ifdef DEBUG
    volatile uint64_t counter;
    volatile int rounds CACHE_ALIGN;
#endif
} OsciStruct;

void OsciInit(OsciStruct *l, uint32_t nthreads, uint32_t fibers_per_thread);
void OsciThreadStateInit(OsciThreadState *st_thread, OsciStruct *l, int pid);
RetVal OsciApplyOp(OsciStruct *l, OsciThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);

#endif
