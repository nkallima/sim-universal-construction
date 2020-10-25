#ifndef _FAM_H_

#define _FAM_H_

#include <stdint.h>
#include <config.h>

typedef union ObjectState {
    uint64_t state;
    float state_f;
} ObjectState;

inline static RetVal fetchAndMultiply(void *state, ArgVal arg, int pid) {
    ObjectState *obj = (ObjectState *)state;
    ObjectState res;

    res.state_f = obj->state_f;
    //obj->state_f = obj->state_f * 1.00001 + (float)arg;
    obj->state_f *= 1.00001;

    return (RetVal)res.state;
}

#endif
