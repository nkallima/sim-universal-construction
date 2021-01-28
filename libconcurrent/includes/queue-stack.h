#ifndef _QUEUE_STACK_H_
#define _QUEUE_STACK_H_

#include <limits.h>

typedef struct Node {
    Object val;
    volatile struct Node *next;
} Node;

#define GUARD_VALUE     LONG_MIN
#define EMPTY_QUEUE     (GUARD_VALUE + 1)
#define EMPTY_STACK     (GUARD_VALUE + 1)
#define ENQUEUE_SUCCESS 0
#define ENQUEUE_FAIL    -1
#define PUSH_SUCCESS    0
#define PUSH_FAIL       -1

#endif