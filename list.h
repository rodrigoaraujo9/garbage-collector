#ifndef LIST_H
#define LIST_H

#include "bool.h"
#include <stddef.h>

// Adapted notion of list.
// "General List" will be used as "hash table"
//    - for the list nodes in here, data is sizeofspace ++ list (list of pointers) -> use a struct
//    - ordered by size / addr (*)
// "List of Pointers"
//    - for list nodes data is pointer

typedef struct ListNode_ {
  void *data; // used to pass list of pointers
  unsigned int size;
  struct ListNode_ *next;
} ListNode;

typedef struct List_ {
  unsigned int size;
  ListNode *first;
} List;

void list_init(List *list);

int list_size(List *list);

bool list_isempty(List *list);

void list_addfirst(List *list, void *data, size_t size);

void list_addlast(List *list, void *data, size_t size);

void *list_getfirst(List *list);

void *list_getlast(List *list);

void *list_get(List *list, int index);

void list_removefirst(List *list);

void list_removelast(List *list);

void list_print(List *list);

#endif
