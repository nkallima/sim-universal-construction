#include <barrier.h>
#include <primitives.h>
#include <threadtools.h>

inline void synchBarrierSet(SynchBarrier *bar, uint32_t n) {
    bar->arrive = n;
    bar->leave = n;
    bar->arrive_flag = true;
    bar->leave_flag = true;
    bar->val_at_set = n;
    FullFence();
}

inline void synchBarrierWait(SynchBarrier *bar) {
    if (FAA32(&bar->arrive, -1) > 1) {
        while (bar->arrive_flag)
            synchResched();
    } else {
        bar->arrive = bar->val_at_set;
        NonTSOFence();
        bar->leave_flag = true;
        NonTSOFence();
        bar->arrive_flag = false;
        FullFence();
    }

    FullFence();
    if (FAA32(&bar->leave, -1) > 1) {
        while (bar->leave_flag)
            synchResched();
    } else {
        bar->leave = bar->val_at_set;
        NonTSOFence();
        bar->arrive_flag = true;
        NonTSOFence();
        bar->leave_flag = false;
        FullFence();
    }
}

inline void synchBarrierLeave(SynchBarrier *bar) {
    FAA32(&bar->arrive, -1);
    FAA32(&bar->leave, -1);
}

inline void synchBarrierLastLeave(SynchBarrier *bar) {
    synchBarrierLeave(bar);
    while (bar->arrive != 0 && bar->leave != 0) {
        synchResched();
    }
}
