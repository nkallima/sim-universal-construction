#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "pool.h"

#define  GUARD          INT_MIN
#define  PUSH           1
#define  POP            2

const int COUNTER_BOUND = 3 * N_THREADS;

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
    LockNode *next_node[2];
    int toggle;
    PoolStruct pool_node;
} ThreadState;

typedef struct LockStruct {
    volatile LockNode *Tail CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} LockStruct;

typedef struct Node {
    Object val;
    volatile struct Node *next;
} Node;

Node guard = {GUARD, null};
volatile Node *head CACHE_ALIGN;

inline static void threadStateInit(ThreadState *st_thread, LockStruct *l, int pid) {
    st_thread->next_node[0] = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockNode));
    st_thread->next_node[1] = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockNode));
    st_thread->toggle = 0;
	init_pool(&st_thread->pool_node, sizeof(Node));
}

inline static RetVal push(ThreadState *st_thread, ArgVal arg) {
    Node *node = alloc_obj(&st_thread->pool_node);
    node->next = head;
    node->val = arg;
    head = node;
    return -1;
}

inline static RetVal pop(void) {
     Node *node = (Node *)head;
     if (head->next == null)
         return -1;
     else { 
        head = head->next;
        return node->val;
     }
}

inline static RetVal applyOp(LockStruct *l, ThreadState *st_thread, ArgVal arg, int pid) {
    volatile LockNode *p;
    volatile LockNode *cur;
    register LockNode *next_node;
    register int toggle, counter;

    toggle = st_thread->toggle;
    next_node = st_thread->next_node[toggle];
    StorePrefetch(next_node);
    st_thread->toggle = 1 - toggle;
    next_node->next = null;
    next_node->locked = true;
    next_node->completed = false;

    cur = (LockNode * volatile)SWAP(&l->Tail, next_node);
    cur->next = (LockNode *)next_node;
    cur->arg_ret = arg;
    st_thread->next_node[toggle] = (LockNode *)cur;

    while (cur->locked) {                      // spinning
#ifdef sparc
#    if N_THREADS <= 128
        FullFence();
        FullFence();
        FullFence();
        FullFence();
#    else
        sched_yield();
#    endif
#elif N_THREADS > USE_CPUS
        sched_yield();
#else
        ;
#endif
    }

    p = cur;                                // I am not been helped
#ifdef DEBUG
    l->rounds++;
#endif

    if (cur->completed)                     // I have been helped
        return cur->arg_ret;

    while (p->next != null && counter < COUNTER_BOUND) {
        ReadPrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        if (p->arg_ret == PUSH) {
		    push(st_thread, p->arg_ret);
		} else {
		    p->arg_ret = pop();
		}
        p->completed = true;
        p->locked = false;
        p = p->next;
    }
    p->locked = false;                      // Unlock the next one

    return cur->arg_ret;
}

void lock_init(LockStruct *l) {
    l->Tail = getAlignedMemory(CACHE_LINE_SIZE, sizeof(LockNode));
    l->Tail->next = null;
    l->Tail->locked = false;
    l->Tail->completed = false;
#ifdef DEBUG
    l->rounds = l->counter =0;
#endif
    StoreFence();
}

#endif
