// This is a slightly modified version of the LCRQ queue implementation by Adam Morrison
// and Yehuda Afek provided in http://mcg.cs.tau.ac.il/projects/lcrq.
// LCRQ is presented in "Fast concurrent queues for x86 processors", by Adam Morrison,
// and Yehuda Afek, in PPoPP 2013.
// This modified version has some minor modifications in order to be compatible with the 
// latest version of the Synch framework.
// Notice that this algorithm performs exceptionally well in X86 machines that natively
// support 128-bit Compare&Swap instructions. It is also tested in ARM-V8 and in RISCV machines.
// This modified version seems to have very similar performance as the original one.
// The original version of LCRQ is distributed under the New BSD Licence (LGPL-2.1 is 
// compatible with New BSD Licence, more on https://www.gnu.org/licenses/license-compatibility.html).


// Copyright (c) 2013, Adam Morrison and Yehuda Afek.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//  * Neither the name of the Tel Aviv University nor the names of the
//    author of this software may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <primitives.h>
#include <lcrq.h>

inline static int is_empty(uint64_t v) __attribute__ ((pure));
inline static uint64_t node_index(uint64_t i) __attribute__ ((pure));
inline static uint64_t set_unsafe(uint64_t i) __attribute__ ((pure));
inline static uint64_t node_unsafe(uint64_t i) __attribute__ ((pure));
inline static uint64_t tail_index(uint64_t t) __attribute__ ((pure));
inline static int crq_is_closed(uint64_t t) __attribute__ ((pure));
inline static void fix_state(RingQueue *rq);

#ifdef DEBUG
inline static void count_closed_crq(LCRQThreadState *thread_state) {
    thread_state->mycloses++;
}

inline static void count_unsafe_node(LCRQThreadState *thread_state) {
    thread_state->myunsafes++;
}
#else
inline static void count_closed_crq(LCRQThreadState *thread_state) {
}
inline static void count_unsafe_node(LCRQThreadState *thread_state) {
}
#endif

inline static void init_ring(RingQueue *r) {
    int i;

    for (i = 0; i < RING_SIZE; i++) {
        r->array[i].val = -1;
        r->array[i].idx = i;
    }

    r->head = r->tail = 0;
    r->next = NULL;
}

inline static int is_empty(uint64_t v)  {
    return (v == (uint64_t)-1);
}

inline static uint64_t node_index(uint64_t i) {
    return (i & ~(1ull << 63));
}

inline static uint64_t set_unsafe(uint64_t i) {
    return (i | (1ull << 63));
}

inline static uint64_t node_unsafe(uint64_t i) {
    return (i & (1ull << 63));
}

inline static uint64_t tail_index(uint64_t t) {
    return (t & ~(1ull << 63));
}

inline static int crq_is_closed(uint64_t t) {
    return (t & (1ull << 63)) != 0;
}

inline static void fix_state(RingQueue *rq) {
    while (true) {
        uint64_t t = synchFAA64(&rq->tail, 0);
        uint64_t h = synchFAA64(&rq->head, 0);

        if (synchUnlikely(rq->tail != t)) continue;

        if (h > t) {
            if (synchCAS64(&rq->tail, t, h)) break;
            continue;
        }
        break;
    }
}

inline static int close_crq(RingQueue *rq, const uint64_t t, const int tries) {
    if (tries < 10)
        return synchCAS64(&rq->tail, t + 1, set_unsafe(t + 1));
    else
        return synchBitTAS64(&rq->tail, 63);
}

void LCRQInit(LCRQStruct *queue, uint32_t nthreads UNUSED_ARG) {
    RingQueue *rq = synchGetMemory(sizeof(RingQueue));
    init_ring(rq);
    queue->head = queue->tail = rq;
}

void LCRQThreadStateInit(LCRQThreadState *thread_state, int pid UNUSED_ARG) {
    thread_state->nrq = NULL;
#ifdef HAVE_HPTRS
    thread_state->hazardptr = NULL;
#endif
}

void LCRQEnqueue(LCRQStruct *queue, LCRQThreadState *thread_state, ArgVal arg, int pid UNUSED_ARG) {
    int try_close = 0;

    while (true) {
        RingQueue *rq = queue->tail;

#ifdef HAVE_HPTRS
        synchSWAP(&thread_state->hazardptr, rq);
        if (synchUnlikely(queue->tail != rq)) continue;
#endif

        RingQueue *next = rq->next;

        if (synchUnlikely(next != NULL)) {
            synchCASPTR(&queue->tail, rq, next);
            continue;
        }

        uint64_t t = synchFAA64(&rq->tail, 1);

        if (crq_is_closed(t)) {
alloc:
            if (thread_state->nrq == NULL) {
                thread_state->nrq = synchGetMemory(sizeof(RingQueue));
                init_ring(thread_state->nrq);
            }

            // Solo enqueue
            thread_state->nrq->tail = 1, thread_state->nrq->array[0].val = arg, thread_state->nrq->array[0].idx = 0;

            if (synchCASPTR(&rq->next, NULL, thread_state->nrq)) {
                synchCASPTR(&queue->tail, rq, thread_state->nrq);
                thread_state->nrq = NULL;
                return;
            }
            continue;
        }

        RingNode *cell = &rq->array[t & (RING_SIZE - 1)];
        synchStorePrefetch(cell);

        uint64_t idx = cell->idx;
        uint64_t val = cell->val;

        if (synchLikely(is_empty(val))) {
            if (synchLikely(node_index(idx) <= t)) {
                if ((synchLikely(!node_unsafe(idx)) || rq->head < t) && synchCAS128((uint64_t *)cell, -1, idx, arg, t)) {
                    return;
                }
            }
        }

        uint64_t h = rq->head;

        if (synchUnlikely((int64_t)(t - h) >= (int64_t)RING_SIZE) && close_crq(rq, t, ++try_close)) {
            count_closed_crq(thread_state);
            goto alloc;
        }
    }
}

RetVal LCRQDequeue(LCRQStruct *queue, LCRQThreadState *thread_state, int pid UNUSED_ARG) {
    while (true) {
        RingQueue *rq = queue->head;
        RingQueue *next;

#ifdef HAVE_HPTRS
        synchSWAP(&thread_state->hazardptr, rq);
        if (synchUnlikely(head != rq)) continue;
#endif

        uint64_t h = synchFAA64(&rq->head, 1);

        RingNode *cell = &rq->array[h & (RING_SIZE - 1)];
        synchStorePrefetch(cell);

        uint64_t tt = 0;
        int r = 0;

        while (true) {
            uint64_t cell_idx = cell->idx;
            uint64_t unsafe = node_unsafe(cell_idx);
            uint64_t idx = node_index(cell_idx);
            uint64_t val = cell->val;

            if (synchUnlikely(idx > h)) break;

            if (synchLikely(!is_empty(val))) {
                if (synchLikely(idx == h)) {
                    if (synchCAS128((uint64_t *)cell, val, cell_idx, -1, (unsafe | h) + RING_SIZE)) return val;
                } else {
                    if (synchCAS128((uint64_t *)cell, val, cell_idx, val, set_unsafe(idx))) {
                        count_unsafe_node(thread_state);
                        break;
                    }
                }
            } else {
                if ((r & ((1ull << 10) - 1)) == 0)
                    tt = rq->tail;

                // Optimization: try to bail quickly if queue is closed.
                int crq_closed = crq_is_closed(tt);
                uint64_t t = tail_index(tt);

                if (synchUnlikely(unsafe)) {  // Nothing to do, move along
                    if (synchCAS128((uint64_t *)cell, val, cell_idx, val, (unsafe | h) + RING_SIZE)) break;
                } else if (t < h + 1 || r > 200000 || crq_closed) {
                    if (synchCAS128((uint64_t *)cell, val, idx, val, h + RING_SIZE)) {
                        if (r > 200000 && tt > RING_SIZE) synchBitTAS64(&rq->tail, 63);
                        break;
                    }
                } else {
                    ++r;
                }
            }
        }

        if (tail_index(rq->tail) <= h + 1) {
            fix_state(rq);
            // try to return empty
            next = rq->next;
            if (next == NULL)
                return EMPTY_QUEUE;  // EMPTY
            if (tail_index(rq->tail) <= h + 1) synchCASPTR(&queue->head, rq, next);
        }
    }
}
