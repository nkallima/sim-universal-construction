/// @file lcrq.h
/// @author Adam Morrison and Yehuda Afek
/// @brief This is a slightly modified version of the LCRQ queue implementation by Adam Morrison
/// and Yehuda Afek provided in http://mcg.cs.tau.ac.il/projects/lcrq.
///
/// LCRQ is presented in "Fast concurrent queues for x86 processors", by Adam Morrison,
/// and Yehuda Afek, in PPoPP 2013.
/// This modified version has some minor modifications in order to be compatible with the 
/// latest version of the Synch framework.
/// Notice that this algorithm performs exceptionally well in X86 machines that natively
/// support 128-bit Compare&Swap instructions. It is also tested in ARM-V8 and in RISCV machines.
/// This modified version seems to have very similar performance as the original one.
/// The original version of LCRQ is distributed under the New BSD Licence (LGPL-2.1 is 
/// compatible with New BSD Licence, more on https://www.gnu.org/licenses/license-compatibility.html).
/// @copyright Copyright (c) 2021

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

#include <stdint.h>

#include <config.h>
#include <queue-stack.h>

// Definition: RING_POW
// --------------------
// The LCRQ's ring size will be 2^{RING_POW}.
#ifndef RING_POW
#define RING_POW        (14)
#endif
#define RING_SIZE       (1ULL << RING_POW)


// Definition: HAVE_HPTRS
// --------------------
// Define to enable hazard pointer setting for safe memory
// reclamation.  You'll need to integrate this with your
// hazard pointers implementation.
//#define HAVE_HPTRS

typedef struct RingNode {
    volatile uint64_t val;
    volatile uint64_t idx;
    char pad[PAD_CACHE(2 * sizeof(uint64_t))];
} RingNode;

typedef struct RingQueue {
    volatile int64_t head S_CACHE_ALIGN;
    volatile int64_t tail S_CACHE_ALIGN;
    struct RingQueue *next S_CACHE_ALIGN;
    RingNode array[RING_SIZE] S_CACHE_ALIGN;
} RingQueue;

/// @brief LCRQStruct stores the state of an instance of the LCRQ concurrent queue implementation.
/// LCRQStruct should be initialized using the LCRQStructInit function.
typedef struct LCRQStruct {
    /// @brief A pointer to the head Ring.
    RingQueue *head;
    /// @brief A pointer to the tail Ring.
    RingQueue *tail;
#ifdef DEBUG
    uint64_t closes;
    uint64_t unsafes;
#endif
} LCRQStruct;

/// @brief LCRQThreadState stores each thread's local state for a single instance of LCRQ.
/// For each instance of LCRQ, a discrete instance of LCRQThreadState should be used.
typedef struct LCRQThreadState {
    RingQueue *nrq;
#ifdef HAVE_HPTRS
    RingQueue *hazardptr;
#endif
#ifdef DEBUG
    uint64_t mycloses;
    uint64_t myunsafes;
#endif
} LCRQThreadState;


/// @brief This function initializes an instance of the LCRQ concurrent queue implementation.
/// This function should be called once (by a single thread) before any other thread tries to
/// apply any enqueue or dequeue operation.
///
/// @param queue A pointer to an instance of the LCRQ concurrent queue implementation.
/// @param nthreads The number of threads that will use the LCRQ concurrent queue implementation.
void LCRQInit(LCRQStruct *queue, uint32_t nthreads);

/// @brief This function should be called once before the thread applies any operation to the LCRQ concurrent queue implementation.
///
/// @param thread_state A pointer to thread's local state of LCRQ.
/// @param pid The pid of the calling thread.
void LCRQThreadStateInit(LCRQThreadState *thread_state, int pid);

/// @brief This function adds (i.e. enqueues) a new element to the back of the queue.
/// This element has a value equal with arg.
///
/// @param queue A pointer to an instance of the LCRQ concurrent queue implementation.
/// @param thread_state A pointer to thread's local state of LCRQ.
/// @param arg The enqueue operation will insert a new element to the queue with value equal to arg.
/// @param pid The pid of the calling thread.
void LCRQEnqueue(LCRQStruct *queue, LCRQThreadState *thread_state, ArgVal arg, int pid);

/// @brief This function removes (i.e. dequeues) an element from the front of the queue and returns its value.
///
/// @param queue A pointer to an instance of the LCRQ concurrent queue implementation.
/// @param thread_state A pointer to thread's local state of LCRQ.
/// @param pid The pid of the calling thread.
/// @return The value of the removed element.
RetVal LCRQDequeue(LCRQStruct *queue, LCRQThreadState *thread_state, int pid);