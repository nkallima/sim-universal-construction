#include <backoff.h>
#include <threadtools.h>
#include <sched.h> // sched_yield();

void synchInitBackoff(SynchBackoffStruct *b, unsigned base_bits, unsigned cap_bits, unsigned shift_bits) {
    b->backoff_base_bits = base_bits;
    b->backoff_cap_bits = cap_bits;
    b->backoff_shift_bits = shift_bits;
    b->backoff_base = (1 << b->backoff_base_bits) - 1;
    b->backoff_cap = (1 << b->backoff_cap_bits) - 1;
    b->backoff_addend = (1 << b->backoff_shift_bits) - 1;
    b->backoff = b->backoff_base;
}

void synchResetBackoff(SynchBackoffStruct *b) {
    b->backoff = b->backoff_base;
}

void synchBackoffDelay(SynchBackoffStruct *b) {
    if (synchIsSystemOversubscribed()) {
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

void synchBackoffReduce(SynchBackoffStruct *b) {
    b->backoff >>= b->backoff_shift_bits;
    if (b->backoff < b->backoff_base)
        b->backoff = b->backoff_base;
}

void synchBackoffIncrease(SynchBackoffStruct *b) {
    b->backoff <<= b->backoff_shift_bits;
    b->backoff += b->backoff_addend;
    if (b->backoff > b->backoff_cap)
        b->backoff = b->backoff_cap;
}
