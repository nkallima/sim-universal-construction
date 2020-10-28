#ifndef _OYAMA_H_
#define _OYAMA_H_

#include <config.h>
#include <primitives.h>
#include <fastrand.h>
#include <threadtools.h>

typedef struct HalfOyamaAnnounceNode {
    volatile struct OyamaAnnounceNode *next;
    volatile ArgVal arg_ret;
    int32_t pid;
    volatile bool completed;
} HalfOyamaAnnounceNode;

typedef struct OyamaAnnounceNode {
    volatile struct OyamaAnnounceNode *next;
    volatile ArgVal arg_ret;
    int32_t pid;
    volatile bool completed;
    char align[PAD_CACHE(sizeof(HalfOyamaAnnounceNode))];
} OyamaAnnounceNode;

typedef struct OyamaStruct {
    volatile int32_t lock CACHE_ALIGN;
    volatile OyamaAnnounceNode *tail CACHE_ALIGN;
    uint32_t nthreads CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} OyamaStruct;

typedef struct OyamaThreadState {
    OyamaAnnounceNode my_node;
} OyamaThreadState;

RetVal OyamaApplyOp(volatile OyamaStruct *l, OyamaThreadState *th_state, RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid);
void OyamaThreadStateInit(OyamaThreadState *th_state);
void OyamaInit(OyamaStruct *l, uint32_t nthreads);

#endif
