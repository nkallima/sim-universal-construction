#ifndef _BACKOFF_H_

#define _BACKOFF_H_

#include "rand.h"


#define BHIS                       4
#define MAX_COUNTER                8


typedef struct BackoffStruct {
    unsigned backoff;
    unsigned backoff_base_bits;
    unsigned backoff_cap_bits;
    unsigned backoff_shift_bits;
    unsigned backoff_base;
    unsigned backoff_cap;
    unsigned backoff_addend;
    unsigned help[BHIS];
    unsigned counter;
    unsigned rnd;
    unsigned prev_avg;
} BackoffStruct;

inline static void init_backoff(BackoffStruct *b, unsigned base_bits, unsigned cap_bits, unsigned shift_bits) {
    int i;

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
    for (i = 0; i < BHIS; i++)
        b->help[i] = 0;
}

inline static void reset_backoff(BackoffStruct *b) {
    b->backoff = b->backoff_base;
}


inline static void backoff_delay(BackoffStruct *b) {
#ifdef DISABLE_BACKOFF
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
    pthread_yield();
#elif N_THREADS <= USE_CPUS
    volatile unsigned i;

    for (i=0; i < b->backoff; i++)
        ;
#else
    volatile unsigned i;

    sched_yield();
    for (i=0; i < b->backoff; i++)
        ;
#endif
}

inline static void backoff_reduce(BackoffStruct *b) {
    b->backoff >>= b->backoff_shift_bits;
    b->backoff |= b->backoff_base;
    b->backoff &= b->backoff_cap;
}

inline static void backoff_increase(BackoffStruct *b) {
    b->backoff <<= b->backoff_shift_bits;
    b->backoff += b->backoff_addend;
    b->backoff &= b->backoff_cap;
    b->backoff |= b->backoff_base;
}


inline static void backoffCalculate(BackoffStruct *b, int active) {
#ifdef DISABLE_BACKOFF
    ;
#else
    unsigned mod, i, avg;

    b->rnd++;
    mod = b->rnd % BHIS;
    b->help[mod] = active;
    if (mod == 0) {
        avg = 0;
        for (i = 0; i < BHIS; i++)
            avg += b->help[i];
        
        if (avg < BHIS*(N_THREADS/2))
            b->counter++;
        else if (avg == b->prev_avg)
            b->counter--;
        if (b->counter == MAX_COUNTER) {
            backoff_increase(b);
            b->counter = MAX_COUNTER/2 + 1;
        } else if (b->counter == 0) {
            backoff_reduce(b);
            b->counter = MAX_COUNTER/2 + 1;
        } 
        b->prev_avg = avg;
    }
#endif
}

#endif

