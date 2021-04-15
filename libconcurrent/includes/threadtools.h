#ifndef _THREAD_H_
#define _THREAD_H_

#include <config.h>
#include <stdint.h>
#include <stdbool.h>

#define _DONT_USE_UTHREADS_ 1

void setThreadId(int32_t id);
int threadPin(int32_t cpu_id);
int StartThreadsN(uint32_t nthreads, void *(*func)(void *), uint32_t uthreads);
void JoinThreadsN(uint32_t nthreads);

inline int32_t getThreadId(void);
inline int32_t getPreferedCore(void);
inline uint32_t preferedCoreOfThread(uint32_t pid);
inline uint32_t getNCores(void);
inline void resched(void);
inline bool isSystemOversubscribed(void);

#endif
