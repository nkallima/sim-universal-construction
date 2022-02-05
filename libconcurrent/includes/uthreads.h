/// @file uthreads.h
/// @brief This file exposes the API by the Synch framework for creating fibers inside a posix thread.
#undef _FORTIFY_SOURCE

#ifndef _UTHREADS_H_
#define _UTHREADS_H_

#include <stdlib.h>
#include <ucontext.h>
#include <setjmp.h>

#include <config.h>
#include <primitives.h>

/// @brief This function initiates the fiber environment inside a posix thread.
/// @param max The maximum number of fibers that the current posix thread could create.
void synchInitFibers(int max);

/// @brief This function gives the control of the processor to the next fiber of the current posix thread.
void synchFiberYield(void);

/// @brief This function spawns a new fiber inside the current posix thread.
/// @param func A pointer to a function that newely spawned fiber will execute after its creation.
/// @param arg An argument passed to the newely created thread.
int synchSpawnFiber(void *(*func)(void *), long arg);

/// @brief Wait for all of the fibers that are running inside the current posix thread to finish their execution.
void synchWaitForAllFibers(void);

/// @brief This function returns an index of the running fiber in the current posix thread, i.e.
/// an integer number between 0 and N-1, where N is the number of active fibers in the cirrent posix thread.
int32_t synchCurrentFiberIndex(void);

#endif
