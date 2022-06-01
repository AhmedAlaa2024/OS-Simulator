#include "priority_queue.h"
#define DEBUGGING   1

int main(void) {
    #if (DEBUGGING == 1)
    printf("Warning: Debugging Mode has been enabled!\n\n");
    #endif

    PriorityQueue pq = {pq_newNode(9202141, 0, NULL), 1};

    pq_push(ADDRESS(pq), 9202142, 1);
    pq_push(ADDRESS(pq), 9202143, 1);
    pq_push(ADDRESS(pq), 9202144, 0);
    pq_push(ADDRESS(pq), 9202145, 2);
    pq_push(ADDRESS(pq), 9202146, 3);

    printf("\n");
    
    #if (DEBUGGING == 1)
    printf("The peek now: %d\n\n", pq_peek(ADDRESS(pq)));
    printf("Stage 1: \n");
    pq_print(ADDRESS(pq));
    printf("\n");
    printf("-------------------------------------------------------------\n");
    #endif

    pq_pop(ADDRESS(pq));

    #if (DEBUGGING == 1)
    printf("The peek now: %d\n\n", pq_peek(ADDRESS(pq)));
    printf("Stage 2: \n");
    pq_print(ADDRESS(pq));
    printf("\n");
    printf("-------------------------------------------------------------\n");
    #endif

    pq_pop(ADDRESS(pq));
    pq_pop(ADDRESS(pq));

    #if (DEBUGGING == 1)
    printf("The peek now: %d\n\n", pq_peek(ADDRESS(pq)));
    printf("Stage 3: \n");
    pq_print(ADDRESS(pq));
    printf("\n");
    printf("-------------------------------------------------------------\n");
    #endif

    pq_pop(ADDRESS(pq));
    pq_pop(ADDRESS(pq));

    #if (DEBUGGING == 1)
    printf("The peek now: %d\n\n", pq_peek(ADDRESS(pq)));
    printf("Stage 4: \n");
    pq_print(ADDRESS(pq));
    printf("\n");
    printf("-------------------------------------------------------------\n");
    #endif

    pq_pop(ADDRESS(pq));

    #if (DEBUGGING == 1)
    T temp = pq_peek(ADDRESS(pq));

    if (temp == 0)
        printf("The peek now: NULL\n\n");
    else
        printf("The peek now: %d\n\n", temp);

    printf("Stage 5: \n");
    pq_print(ADDRESS(pq));
    printf("\n");
    printf("-------------------------------------------------------------\n");
    #endif

    return 0;
}