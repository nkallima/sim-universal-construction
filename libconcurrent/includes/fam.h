/// @file fam.h
/// @brief This file implements a very simple Fetch & Multiply serial object.
/// This functionality is used by combining objects or universal construction (e.g. CC-Synch, Osci, etc.) for 
/// providing a concurrent version of this object. The current version of the object stores floats.
/// However, the user can very easily to modify the object in order to store other data type or size;
/// in this case ObjecState struct and the fetchAndMultiply function should be modified.
#ifndef _FAM_H_

#define _FAM_H_

#include <config.h>
#include <stdint.h>

typedef union ObjectState {
    uint64_t state;
    float state_f;
} ObjectState;

/// @brief This is a simple serial implementation of a Fetch&Multiply object.
///
/// @param state Pointer to the stored data.
/// @param arg The argument of the operation.
/// @param pid The pid of the calling thread.
/// @return The result of the serial Fetch&Multiply operation.
inline static RetVal fetchAndMultiply(void *state, ArgVal arg, int pid) {
    ObjectState *obj = (ObjectState *)state;
    ObjectState res;

    res.state_f = obj->state_f;
    // obj->state_f = obj->state_f + (float)arg;
    obj->state_f *= 1.000001;

    return (RetVal)res.state;
}

#endif
