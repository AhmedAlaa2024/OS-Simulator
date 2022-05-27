#include "priority_queue.h"

/* Create a new node, if there is no next, you can pass NULL */
Node* pq_newNode(T data, int priority, T* next){
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = data;
    node->priority = priority;
    node->next = NULL;

    return node;
}

/* Return the value at head */
T pq_peek(PriorityQueue* pq) {
    if (pq_isEmpty(pq))
        return 0;
    return pq->Head->data;
}

/* Remove and return the element with the highest priority from the priority queue */
T pq_pop(PriorityQueue* pq) {
    if (pq_isEmpty(pq))
        return 0;
        
    Node *temp = pq->Head;
    pq->Head = pq->Head->next;
    T data = temp->data;
    free(temp);
    temp = NULL;
    (pq->num_of_nodes)--;
    return data;
}

/* Push a new element according to priority */
void pq_push(PriorityQueue *pq, T data, int priority) {
    Node *walker = pq->Head;

    /* Create a new node */
    Node *temp = pq_newNode(data, priority, NULL);

    if(pq_isEmpty(pq)) {
        pq->Head = temp;
        (pq->num_of_nodes)++;
        return;
    }

    /* Special Case: The new node has a greater priority than the head (low-priority number means high priority) */
    if (priority < walker->priority) {
        temp->next = walker;
        pq->Head = temp;
    }
    else {
        /* Traverse until you find the suitable position (low-priority number means high priority) */
        while ((walker->next != NULL) && (walker->next->priority <= priority))
            walker = walker->next;

        /* Found the suitable position */
        temp->next = walker->next;
        walker->next = temp;
    }

    (pq->num_of_nodes)++;
}

/* Chech if the list is empty */
bool pq_isEmpty(PriorityQueue* pq) {
    return (pq->Head == NULL);
}

int pq_getLength(PriorityQueue* pq)
{
    return pq->num_of_nodes;
}

/* Print the priority queue in order to trace its elements */
// void pq_print(PriorityQueue* pq) {
//     Node *temp = pq->Head;

//     if (pq_isEmpty(pq))
//         printf("Priority Queue is empty!");

//     while (temp != NULL) {
//         printf("Node { ");
//         printf("Data: %d,\n", temp->data);
//         printf("key: %d,\n", temp->key);
//         printf("priority: %d\n}\n\n", temp->priority);
//         temp = temp->next;
//     }
// }