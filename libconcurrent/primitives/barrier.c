#include <barrier.h>

inline void BarrierInit(Barrier *bar, int n) {
    bar->v = n;
    FullFence();
}

inline void BarrierWait(Barrier *bar) {
    FAA32(&bar->v, -1);
    while (bar->v != 0)
        resched();
}

inline void BarrierLeave(Barrier *bar) {
    FAA32(&bar->v, -1);
}