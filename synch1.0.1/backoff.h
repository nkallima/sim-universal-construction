#ifndef _BACKOFF_H_

#define _BACKOFF_H_
#include <sys/select.h>
#include "rand.h"


#define BHIS                       4
#define MAX_COUNTER                4


typedef struct BackoffStruct {
    unsigned backoff;
    unsigned backoff_base_bits;
    unsigned backoff_cap_bits;
    unsigned backoff_shift_bits;
    unsigned backoff_base;
    unsigned backoff_cap;
    unsigned backoff_addend;
    unsigned help;
    unsigned counter;
    unsigned rnd;
    unsigned prev_avg;
} BackoffStruct;

inline static void init_backoff(BackoffStruct *b, unsigned base_bits, unsigned cap_bits, unsigned shift_bits) {
    b->backoff_base_bits = base_bits;
    b->backoff_cap_bits = cap_bits;
    b->backoff_shift_bits = shift_bits;
    b->backoff_base = (1 << b->backoff_base_bits) - 1;
    b->backoff_cap = (1 << b->backoff_cap_bits) - 1;
    b->backoff_addend = (1 << b->backoff_shift_bits) - 1;
    b->backoff = b->backoff_base;
    b->rnd = 0;
    b->counter = 0;
    b->prev_avg = 0;
    b->help = 0;
}

inline static void reset_backoff(BackoffStruct *b) {
    b->backoff = b->backoff_base;
}


inline static void backoff_delay(BackoffStruct *b) {
#if N_THREADS > USE_CPUS
#   ifdef sparc
    sched_yield();
    sched_yield();
    sched_yield();
    sched_yield();
    sched_yield();
#   else
    sched_yield();
#   endif
#elif defined(DISABLE_BACKOFF)
    ;
#else
    volatile unsigned i;

    for (i = 0; i < b->backoff; i++)
        ;

    b->backoff <<= b->backoff_shift_bits;
    b->backoff += b->backoff_addend;
    b->backoff &= b->backoff_cap;
#endif
}


inline static void backoff_play(BackoffStruct *b) {
#ifdef DISABLE_BACKOFF
    ;
#elif N_THREADS >= 2 * USE_CPUS
    sched_yield();
#elif N_THREADS <= USE_CPUS
    volatile unsigned i;

    for (i=0; i < b->backoff; i++)
        ;
#else
    volatile unsigned i;

    pthread_yield();
    for (i=0; i < b->backoff; i++)
        ;
#endif
}

inline static void backoff_reduce(BackoffStruct *b) {
    //b->backoff >>= b->backoff_shift_bits;
    b->backoff = (int)((float)b->backoff * 0.5);
    if (b->backoff < b->backoff_base)
        b->backoff = b->backoff_base;
}

inline static void backoff_increase(BackoffStruct *b) {
    //b->backoff <<= b->backoff_shift_bits;
    //b->backoff += b->backoff_addend;
    b->backoff = (int)((float)b->backoff * 2);
    if (b->backoff > b->backoff_cap)
        b->backoff = b->backoff_cap;
}


inline static void backoffCalculate(BackoffStruct *b, int active) {
#ifdef DISABLE_BACKOFF
    ;
#else
    unsigned mod, avg;

    b->rnd++;
    mod = b->rnd % BHIS;
    b->help += active;
    if (mod == 0) {
        avg = b->help / BHIS;
        
        if (avg < BHIS*(3*N_THREADS/2)) {
            b->counter++;
            backoff_increase(b);
        } else if (avg == b->prev_avg) {
            b->counter--;
            backoff_reduce(b);
        }
        b->prev_avg = avg;
        b->help = 0;
    }
#endif
}

inline static void backoffReCalc(BackoffStruct *b, bool c1, bool c2, int help) {
    const int help_factor = 0.2 * N_THREADS;

    if (c1) {
        backoff_reduce(b);
    } else if (c2) {
        if (help < help_factor)
            backoff_increase(b);
        else backoff_reduce(b);
    } else {
        reset_backoff(b);
    }
}

#endif

