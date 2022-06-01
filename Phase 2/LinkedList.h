#ifndef _LINKEDLIST_H
#define _LINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>
#include "headers.h"

/* This is the node definition */
typedef struct ll_node {
    Segment* data;
    int key;

    struct ll_node* next;
} ll_Node;

/* This is the priority queue definition which each node corresponds to one process */
typedef struct {
    ll_Node *Head;
    int num_of_nodes;
} LinkedList;

/* Construct an empty LinkedList */
LinkedList* ll_constructor(void);

/* Create a new node, if there is no next, you can pass NULL */
ll_Node* ll_newNode(Segment* data, ll_Node* next);

/* Insert a new element at the front of the list */
void ll_insert(LinkedList *ll, Segment* data, int key);

/* Remove and return the element at the end of the list */
ll_Node* ll_remove(LinkedList* ll, int key);

/* Search for the node carring a certain data */
ll_Node* ll_search(LinkedList* ll, Segment* data);

/* Chech if the list is empty */
bool ll_isEmpty(LinkedList* ll);

/* Get the number of elements */
int ll_getLength(LinkedList* ll);

/* Print the LinkedList in order to trace its elements */
void ll_print(LinkedList* ll);
#endif