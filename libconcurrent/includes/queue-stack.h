#ifndef _QUEUE_STACK_H_
#define _QUEUE_STACK_H_

#include <limits.h>

#define GUARD_VALUE INT_MIN

typedef struct Node {
    Object val;
    volatile struct Node *next;
} Node;


#endif