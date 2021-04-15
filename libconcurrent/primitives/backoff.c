#include <backoff.h>
#include <threadtools.h>
#include <sched.h> // sched_yield();

void init_backoff(BackoffStruct *b, unsigned base_bits, unsigned cap_bits, unsigned shift_bits) {
    b->backoff_base_bits = base_bits;
    b->backoff_cap_bits = cap_bits;
    b->backoff_shift_bits = shift_bits;
    b->backoff_base = (1 << b->backoff_base_bits) - 1;
    b->backoff_cap = (1 << b->backoff_cap_bits) - 1;
    b->backoff_addend = (1 << b->backoff_shift_bits) - 1;
    b->backoff = b->backoff_base;
}

void reset_backoff(BackoffStruct *b) {
    b->backoff = b->backoff_base;
}

void backoff_delay(BackoffStruct *b) {
    if (isSystemOversubscribed()) {
        sched_yield();
    } else {
#ifndef DISABLE_BACKOFF
        volatile unsigned i;

        for (i = 0; i < b->backoff; i++)
            ;

        b->backoff <<= b->backoff_shift_bits;
        b->backoff += b->backoff_addend;
        b->backoff &= b->backoff_cap;
#else
        ;
#endif
    }
}

void backoff_reduce(BackoffStruct *b) {
    b->backoff >>= b->backoff_shift_bits;
    if (b->backoff < b->backoff_base)
        b->backoff = b->backoff_base;
}

void backoff_increase(BackoffStruct *b) {
    b->backoff <<= b->backoff_shift_bits;
    b->backoff += b->backoff_addend;
    if (b->backoff > b->backoff_cap)
        b->backoff = b->backoff_cap;
}
