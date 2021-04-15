#ifndef _BARRIER_H_
#define _BARRIER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile int32_t arrive;
    volatile int32_t leave;
    volatile bool arrive_flag;
    volatile bool leave_flag;
    volatile int32_t val_at_set;
} Barrier;

inline void BarrierSet(Barrier *bar, uint32_t n);
inline void BarrierWait(Barrier *bar);
inline void BarrierLeave(Barrier *bar);
inline void BarrierLastLeave(Barrier *bar);

#endif