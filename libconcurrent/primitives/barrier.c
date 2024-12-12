#include <barrier.h>
#include <primitives.h>
#include <threadtools.h>

void synchBarrierSet(SynchBarrier *bar, uint32_t n) {
    bar->arrive = n;
    bar->leave = n;
    bar->arrive_flag = true;
    bar->leave_flag = true;
    bar->val_at_set = n;
    synchFullFence();
}

void synchBarrierWait(SynchBarrier *bar) {
    if (synchFAA32(&bar->arrive, -1) > 1) {
        while (bar->arrive_flag)
            synchResched();
    } else {
        bar->arrive = bar->val_at_set;
        synchNonTSOFence();
        bar->leave_flag = true;
        synchNonTSOFence();
        bar->arrive_flag = false;
        synchFullFence();
    }

    synchFullFence();
    if (synchFAA32(&bar->leave, -1) > 1) {
        while (bar->leave_flag)
            synchResched();
    } else {
        bar->leave = bar->val_at_set;
        synchNonTSOFence();
        bar->arrive_flag = true;
        synchNonTSOFence();
        bar->leave_flag = false;
        synchFullFence();
    }
}

void synchBarrierLeave(SynchBarrier *bar) {
    synchFAA32(&bar->arrive, -1);
    synchFAA32(&bar->leave, -1);
}

void synchBarrierLastLeave(SynchBarrier *bar) {
    synchBarrierLeave(bar);
    while (bar->arrive != 0 && bar->leave != 0) {
        synchResched();
    }
}
