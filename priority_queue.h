#ifndef _PRIORITY_QUEUE_H
#define _PRIORITY_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include "headers.h"
/* Only you can change the int datatype to your specific datatype */
typedef Process* T;  // it was typedef T struct process* and it gives an error --> why typedef ? --> doaa
#define NULL ((void *)0)

#define true    1
#define false   0
#define bool    char

/* This is the node definition */
typedef struct node {
    T data;
    int key;
    int priority;

    struct node* next;
} Node;

/* This is the priority queue definition which each node corresponds to one process */
typedef struct {
    Node *Head;
    int num_of_nodes;
    int Priority; //(0 -> 10)
} PriorityQueue;

/* Create a new node, if there is no next, you can pass NULL */
Node* pq_newNode(T data, int priority, T* next);

/* Return the value at head */
T pq_peek(PriorityQueue* pq);

/* Remove and return the element with the highest priority from the priority queue */
T pq_pop(PriorityQueue* pq);

/* Push a new element according to priority */
void pq_push(PriorityQueue *pq, T data, int priority);

/* Chech if the list is empty */
bool pq_isEmpty(PriorityQueue* pq);

/* Print the priority queue in order to trace its elements */
// void pq_print(PriorityQueue* pq);
#endif