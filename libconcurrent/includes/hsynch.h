#ifndef _HSYNCH_H_
#define _HSYNCH_H_

#include <system.h>
#include <config.h>
#include <primitives.h>
#include <clh.h>
#include <fastrand.h>

#define HSYNCH_DEFAULT_NUMA_POLICY 0

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
    char align[PAD_CACHE(sizeof(HalfHSynchNode))];
} HSynchNode;

typedef union HSynchNodePtr {
    volatile HSynchNode *ptr;
    char pad[CACHE_LINE_SIZE];
} HSynchNodePtr;

typedef struct HSynchThreadState {
    HSynchNode *next_node;
} HSynchThreadState;

typedef struct HSynchStruct {
    CLHLockStruct *central_lock CACHE_ALIGN;
    HSynchNodePtr *Tail CACHE_ALIGN;
#ifdef DEBUG
    volatile uint64_t counter CACHE_ALIGN;
    volatile int rounds;
#endif
    HSynchNode **nodes CACHE_ALIGN;
    int32_t *node_indexes;
    uint32_t nthreads;
    uint32_t numa_node_size;
    uint32_t numa_nodes;
    bool numa_policy;
} HSynchStruct;

void HSynchStructInit(HSynchStruct *l, uint32_t nthreads, uint32_t numa_regions);
void HSynchThreadStateInit(HSynchStruct *l, HSynchThreadState *st_thread, int pid);
RetVal HSynchApplyOp(HSynchStruct *l, HSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid);
#endif
