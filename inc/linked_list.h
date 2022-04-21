#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>

typedef struct LINKED_LIST_T {
  void* Data;
  struct LINKED_LIST_T *Next;
} LINKED_LIST;

/* Adds a new item to list with it's data initialized to newData.
 * If list is null, a new list will be created.
 *
 * Return Value: Newly added list node.
 */
LINKED_LIST* linked_list_add(void* newData, LINKED_LIST *list);

/* Free all allocated memory for linked list nodes.
 * If freeData is true, also free each node's data.
 */
void linked_list_delete_all(LINKED_LIST *list, bool freeData);

#endif /* LINKED_LIST_H */
