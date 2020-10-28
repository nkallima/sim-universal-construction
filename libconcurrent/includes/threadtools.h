#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(__sun)
#    include <sys/pset.h> // P_PID, processor_bind()
#    include <sys/types.h>
#    include <sys/processor.h>
#    include <sys/procset.h>
#    include <unistd.h> // getpid() */
#elif (defined(__gnu_linux__) || defined(__gnu__linux) || defined(__linux__))
#    include <sched.h> // CPU_SET, CPU_ZERO, cpu_set_t, sched_setaffinity()
#endif

#include <config.h>
#include <system.h>

#define _DONT_USE_UTHREADS_ 1

void setThreadId(int32_t id);
int threadPin(int32_t cpu_id);
int StartThreadsN(uint32_t nthreads, void *(*func)(void *), uint32_t uthreads);
void JoinThreadsN(uint32_t nthreads);

inline int32_t getThreadId(void);
inline int32_t getPreferedCore(void);
inline uint32_t getNCores(void);
inline void resched(void);
inline bool isSystemOversubscribed(void);

#endif
