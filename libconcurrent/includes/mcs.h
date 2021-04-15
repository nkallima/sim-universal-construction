#ifndef _MCS_H_
#define _MCS_H_

#include <config.h>
#include <primitives.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct HalfMCSLockNode {
    volatile bool locked;
    volatile struct MCSLockNode *next;
} HalfMCSLockNode;

typedef struct MCSLockNode {
    volatile bool locked;
    volatile struct MCSLockNode *next;
    char pad[PAD_CACHE(sizeof(HalfMCSLockNode))];
} MCSLockNode;

typedef struct MCSLockStruct {
    volatile MCSLockNode *Tail CACHE_ALIGN;
} MCSLockStruct;

typedef struct MCSThreadState {
    volatile MCSLockNode *MyNode CACHE_ALIGN;
} MCSThreadState;

void MCSLock(MCSLockStruct *l, MCSThreadState *thread_state, int pid);
void MCSUnlock(MCSLockStruct *l, MCSThreadState *thread_state, int pid);
MCSLockStruct *MCSLockInit(void);
void MCSThreadStateInit(MCSThreadState *st_thread, int pid);

#endif
