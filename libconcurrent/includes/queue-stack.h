#ifndef _QUEUE_STACK_H_
#define _QUEUE_STACK_H_

typedef struct Node {
    Object val;
    volatile struct Node *next;
} Node;

#endif