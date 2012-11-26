#ifndef _CCSYNCH_H_
#define _CCSYNCH_H_

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif

#include "config.h"
#include "primitives.h"
#include "rand.h"

const int CCSIM_HELP_BOUND = 3 * N_THREADS;

typedef struct HalfLockNode {
    struct LockNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
} HalfLockNode;

typedef struct LockNode {
    struct LockNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
    int32_t align[PAD_CACHE(sizeof(HalfLockNode))];
} LockNode;

typedef struct ThreadState {
    LockNode *next_node;
    int toggle;
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif
} ThreadState;

typedef struct LockStruct {
    volatile LockNode *Tail CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
    volatile int combiner_counter[N_THREADS];
#endif
} LockStruct;


inline static void threadStateInit(ThreadState *st_thread, LockStruct *l, int pid) {
    st_thread->next_node = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockNode));
#ifdef sun
    st_thread->schedule_control = schedctl_init();
#endif
}

inline static RetVal applyOp(LockStruct *l, ThreadState *st_thread, RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid) {
    volatile LockNode *p;
    volatile LockNode *cur;
    register LockNode *next_node, *tmp_next;
    register int counter = 0;

    next_node = st_thread->next_node;
    next_node->next = null;
    next_node->locked = true;
    next_node->completed = false;

#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
    cur = (LockNode * volatile)SWAP(&l->Tail, next_node);
    cur->arg_ret = arg;
    cur->next = (LockNode *)next_node;
    st_thread->next_node = (LockNode *)cur;
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif

    while (cur->locked) {                      // spinning
#if N_THREADS > USE_CPUS
        sched_yield();
#elif defined(sparc)
        FullFence();
        FullFence();
        FullFence();
        FullFence();
#else
        ;
#endif
    }
    p = cur;                                // I am not been helped
    if (cur->completed)                     // I have been helped
        return cur->arg_ret;
#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
#ifdef DEBUG
    l->rounds++;
    l->combiner_counter[pid] += 1;
#endif
    while (p->next != null && counter < CCSIM_HELP_BOUND) {
        ReadPrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        tmp_next = p->next;
        p->arg_ret = sfunc(p->arg_ret, p->pid);
        p->completed = true;
        p->locked = false;
        p = tmp_next;
    }
    p->locked = false;                      // Unlock the next one
    StoreFence();
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif

    return cur->arg_ret;
}

void lock_init(LockStruct *l) {
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockNode));
    l->Tail->next = null;
    l->Tail->locked = false;
    l->Tail->completed = false;

#ifdef DEBUG
    int i;

    l->rounds = l->counter =0;
    for (i=0; i < N_THREADS; i++)
        l->combiner_counter[i] += 1;
#endif
    StoreFence();
}

#endif
