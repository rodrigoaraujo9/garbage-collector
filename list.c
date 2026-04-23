/*
 * list.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "bool.h"
#include "list.h"
#include "heap.h"


void list_init(List* list) {
   list->first = NULL;
   list->size  = 0;
   return;
}

int list_size(List* list) {
   return list->size;
}

bool list_isempty(List* list) {
   return (list->size == 0);
}

void list_addfirst(List* list, void* data) {
  ListNode* node = (ListNode*)malloc(sizeof(ListNode));
  node->data = data;
  node->next = list->first;
  list->first= node;
  list->size = list->size + 1;
  return;
}

void list_addlast(List* list, void* data) {
  ListNode* node = (ListNode*)malloc(sizeof(ListNode));
  node->data = data;
  node->next = NULL;
  if(list_isempty(list)) {
     list->first = node;
  }
  else {
     ListNode* p = list->first;
     while(p->next != NULL)
        p = p->next;
     p->next = node;
  }
  list->size = list->size + 1;
  return;
}

void* list_getfirst(List* list) {
   if (list_isempty(list))
      return NULL;
   return list->first->data;
}

void* list_getlast(List* list) {
   if(list_isempty(list))
      return NULL;
   ListNode* p = list->first;
   while(p->next != NULL)
      p = p->next;
   return p->data;
}

void* list_get(List* list, int index) {
  if(list_isempty(list))
     return NULL;
  ListNode* p = list->first;
  for(int i = 0; i < index; i++)
      p = p->next;
  return p->data;
}

void list_removefirst(List* list) {
   if(list_isempty(list))
      return;
   list->first = list->first->next;
   list->size = list->size - 1;
   return;
}

void list_removelast(List* list) {
   if (list->size == 1)
      list->first = NULL;
   else {
      ListNode* p = list->first;
      for(int i = 0; i < list->size - 2; i++)
         p = p->next;
      p->next = p->next->next;
   }
   list->size = list->size - 1;
   return;
}

void list_print(List* list) {
   printf("[");
   ListNode* p = list->first;
   while(p != NULL) {
      printf("%p",p->data);
      p = p->next;
      if(p != NULL)
         printf(",");
   }
   printf("]\n");
}
