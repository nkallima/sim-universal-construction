#ifndef _BARRIER_H_
#define _BARRIER_H_

#include <primitives.h>
#include <system.h>
#include <types.h>

typedef volatile int_aligned32_t Barrier;

inline void BarrierInit(Barrier *bar, int n);
inline void BarrierWait(Barrier *bar);
inline void BarrierLeave(Barrier *bar);

#endif