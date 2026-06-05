/*
 * list.h
 */

#ifndef LIST_H
#define LIST_H

#include "bool.h"
#include <stddef.h>

typedef struct ListNode_ {
  void *data;
  unsigned int size;
  struct ListNode_ *next;
} ListNode;

typedef struct List_ {
  int size;
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
