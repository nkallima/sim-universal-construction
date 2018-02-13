#ifndef _BACKOFF_H_
#define _BACKOFF_H_

#include <config.h>
#include <sched.h>
#include <sys/select.h>
#include <fastrand.h>

typedef struct BackoffStruct {
    unsigned backoff;
    unsigned backoff_base_bits;
    unsigned backoff_cap_bits;
    unsigned backoff_shift_bits;
    unsigned backoff_base;
    unsigned backoff_cap;
    unsigned backoff_addend;
} BackoffStruct;

void init_backoff(BackoffStruct *b, unsigned base_bits, unsigned cap_bits, unsigned shift_bits);
void reset_backoff(BackoffStruct *b);
void backoff_delay(BackoffStruct *b);
void backoff_reduce(BackoffStruct *b);
void backoff_increase(BackoffStruct *b);

#endif
