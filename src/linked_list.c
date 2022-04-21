#include <linked_list.h>

#include <assert.h>
#include <stdlib.h>

LINKED_LIST* linked_list_add(void* newData, LINKED_LIST *list) {
  if (!list) {
    list = malloc(sizeof(LINKED_LIST));
    assert(list);
    list->Data = newData;
    list->Next = NULL;
    return list;
  }
  else {
    LINKED_LIST *newNode = malloc(sizeof(LINKED_LIST));
    assert(newNode);
    newNode->Data = newData;
    newNode->Next = list;
    return newNode; 
  }
}

void linked_list_delete_all(LINKED_LIST *list, bool freeData) {
  while (list) {
    LINKED_LIST *node = list;
    list = list->Next;
    if (freeData)
      free(node->Data);
      
    free(node);
  }
}
