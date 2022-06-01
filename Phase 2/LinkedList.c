#include "LinkedList.h"

/* Construct an empty LinkedList */
LinkedList* ll_constructor(void)
{
    LinkedList *ll = (LinkedList*) malloc(sizeof(LinkedList));
    ll->Head = nullptr;
    ll->num_of_nodes = 0;

    return ll;
}

/* Create a new node, if there is no next, you can pass NULL */
ll_Node* ll_newNode(Segment* data, ll_Node* next)
{
    ll_Node* ll_node = (ll_Node*)malloc(sizeof(ll_Node));
    ll_node->data = data;
    ll_node->next = next;

    return ll_node;
}

/* Insert a new element at the certain position of the list */
void ll_insert(LinkedList *ll, Segment* data, int key)
{
}

/* Remove and return the element at the end of the list */
ll_Node* ll_remove(LinkedList* ll, int key)
{
}

/* Search for a node in the linkedList */
ll_Node* ll_search(LinkedList* ll, Segment* data)
{
}

/* Chech if the list is empty */
bool ll_isEmpty(LinkedList* ll)
{
    if (ll->num_of_nodes == 0)
        return true;

    return false;
}

/* Get the number of elements */
int ll_getLength(LinkedList* ll)
{
    return ll->num_of_nodes;
}

/* Print the LinkedList in order to trace its elements */
void ll_print(LinkedList* ll)
{
}