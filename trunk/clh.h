#ifndef _CLH_H_

#define _CLH_H_

#include <stdlib.h>
#include <pthread.h>

#include "primitives.h"

#ifdef POSIX_LOCKS

typedef pthread_mutex_t LockStruct;

inline static void lock(LockStruct *l, int pid) {
    pthread_mutex_lock(l);
}

inline static void unlock(LockStruct *l, int pid) {
    pthread_mutex_unlock(l);
}

LockStruct *lock_init(void) {
    LockStruct *l, tmp = PTHREAD_MUTEX_INITIALIZER;
    int error;

    error = posix_memalign((void *)&l, CACHE_LINE_SIZE, sizeof(LockStruct));
    *l = tmp;
    return l;
}

#else
typedef union LockNode {
    bool locked;
    char align[CACHE_LINE_SIZE];
} LockNode;

typedef struct LockStruct {
    volatile LockNode *Tail CACHE_ALIGN;
    volatile LockNode *MyNode[N_THREADS] CACHE_ALIGN;
    volatile LockNode *MyPred[N_THREADS] CACHE_ALIGN;
} LockStruct;


inline static void lock(LockStruct *l, int pid) {
    l->MyNode[pid]->locked = true;
    l->MyPred[pid] = (LockNode *)GAS(&l->Tail, (void *)l->MyNode[pid]);
    while (l->MyPred[pid]->locked == true)
        ;
}

inline static void unlock(LockStruct *l, int pid) {
    l->MyNode[pid]->locked = false;
    l->MyNode[pid]= l->MyPred[pid];
#ifdef sparc
    StoreFence();
#endif
}

LockStruct *lock_init(void) {
    LockStruct *l;
    int j;

    l = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockStruct));
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockNode));
    l->Tail->locked = false;

    for (j = 0; j < N_THREADS; j++) {
        l->MyNode[j] = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockNode));
        l->MyPred[j] = null;
    }

    return l;
}
#endif

#endif
