#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "heap.h"
#include "globals.h"
#include "collector.h"
#include "list.h"

void heap_init(Heap* heap, unsigned int size, void (*collector)(void *, int)){
    heap->base  = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    heap->size  = size;
    heap->limit = heap->base + size;
    heap->top   = heap->base;
    heap->collector = collector;

  #ifdef MARK_SWEEP
    heap->free = (List*)my_malloc(sizeof(List)); // using my_malloc instead of actual malloc
    list_init(heap->free);
  #endif

  #ifdef COPY_COLLECT
    heap->to = heap->base;
    heap->from = heap->base + heap->size / 2;

    heap->wip_h = NULL;

    heap->top = heap->from;
    heap->limit = heap->from + heap->size / 2;
  #endif

    return;
}

void heap_destroy(Heap* heap) {
    munmap(heap->base, heap->size);
    return;
}

/* Extensions to list to handle free */

void heap_free(void* slot, size_t size) { // data is pointer we want to add to list of pointers
 #ifdef MARK_SWEEP
  if (list_isempty(heap->free) || size == ((slot_pointers *)heap->free->first->data)->slotsize) {
    List *list_of_pointers = ((slot_pointers *)heap->free->first->data)->slots;
    list_addlast(list_of_pointers, slot, sizeof(void *));
    return;
  }
  else if (size < ((slot_pointers *)heap->free->first->data)->slotsize) {
    List *list_of_pointers = my_malloc(sizeof(List));
    list_init(list_of_pointers);
    list_addlast(list_of_pointers, slot, sizeof(void *));

    ListNode* node = (ListNode*)my_malloc(sizeof(ListNode));

    slot_pointers *data = my_malloc(sizeof(slot_pointers));

    data->slots = list_of_pointers;
    data->slotsize = size;
    node->data = data;
    node->size = sizeof(slot_pointers);
    node->next = heap->free->first;
    heap->free->first = node;
  }

  // find place to insert either pointer into existing list of pointers or new list of pointer with our pointer
  ListNode* needle = heap->free->first;

  while (needle->next != NULL && ((slot_pointers *)needle->next->data)->slotsize < size) {
    needle = needle->next;
  }

  if(needle->next != NULL && ((slot_pointers *)needle->next->data)->slotsize == size) { // if this case happens it means that there is already a list for this spacesize to place our pointer
    List *list_of_pointers = ((slot_pointers *)needle->next->data)->slots;
    list_addlast(list_of_pointers, slot, sizeof(void *));
  } else { // handle list creation with pointer here
    List *list_of_pointers = my_malloc(sizeof(List));
    list_init(list_of_pointers);
    list_addlast(list_of_pointers, slot, sizeof(void *));
    // insert in middle of needle and needle->next
    ListNode* node = (ListNode*)my_malloc(sizeof(ListNode));

    slot_pointers *data = my_malloc(sizeof(slot_pointers));

    data->slots = list_of_pointers;
    data->slotsize = size;
    node->data = data;
    node->size = sizeof(slot_pointers);
    node->next = needle->next;
    needle->next = node;
  }
  #else
    printf("*error* to use heap_add_pointer_to_free() activate MARK_SWEEP");
    exit(1);
  #endif
}

// will get the smaller size of slot that accomodates the desired size or NULL if can't get.
// pop the first or last (whatever) of that list and use it (get pointer and remove node cleanly)
// if list_of_pointers becomes empty removes entire node from graph
void *heap_pop_free(unsigned int size) {
  if (list_isempty(heap->free) || size < ((slot_pointers *)heap->free->first->data)->slotsize) {
    return NULL;
  }

  ListNode* needle = heap->free->first;

  while (needle->next != NULL && ((slot_pointers *)needle->next->data)->slotsize < size) {
    needle = needle->next;
  }

  void *slot = list_getfirst(((slot_pointers *)needle->next->data)->slots);
  list_removefirst(((slot_pointers *)needle->next->data)->slots);

  if (list_isempty(((slot_pointers *)needle->next->data)->slots)) {
      list_addlast(heap->free, needle->next->data , ((slot_pointers *)needle->next->data)->slotsize);
      needle->next = needle->next->next;
  }

  return slot;
}


void* my_malloc(unsigned int nbytes) {
    unsigned int total = sizeof(_block_header) + nbytes;

    if (heap->top + total <= heap->limit) {
        _block_header *h = (_block_header *)heap->top;

        h->marked = 0;
        h->size = nbytes;

        h->n_fields = 3;
        h->field_types = (1u << 1) | (1u << 2);

      #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        h->forward = NULL;
      #endif

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

  #ifdef MARK_SWEEP
    _block_header *free = heap->free_h;
    _block_header *prev = NULL;
    while (free!=NULL && free->size < nbytes) {
        prev = free;
        free = free->forward;
    }

    if (free != NULL) {
        free->marked = 0;

        free->n_fields = 3;
        free->field_types = (1u << 1) | (1u << 2);

        if (free == heap->free_h) heap->free_h = free->forward;
        if (prev != NULL) prev->forward = free->forward;

        free->forward = NULL;

        return (char *)free + sizeof(_block_header);
    }


  #endif

    printf("\n\n");
    printf("*my_malloc* not enough space, performing GC...\n");

    clock_t start = clock();

    void *root_objects[max_roots];

    for (int i = 0; i < max_roots; i++)
        root_objects[i] = roots[i].root;

    heap->collector(root_objects, max_roots);

    for (int i = 0; i < max_roots; i++)
        roots[i].root = root_objects[i];

    gc_ticks += clock() - start;

    printf("\n\n");

    if (heap->top + total <= heap->limit) {
        _block_header *h = (_block_header *)heap->top;

        h->marked = 0;
        h->size = nbytes;

        h->n_fields = 3;
        h->field_types = (1u << 1) | (1u << 2);

        h->forward = NULL;

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

  #ifdef MARK_SWEEP
    free = heap->free_h;
    prev = NULL;
    while (free != NULL && free->size < nbytes) {
        prev = free;
        free = free->forward;
    }

    if (free != NULL) {
        free->marked = 0;

        free->n_fields = 3;
        free->field_types = (1u << 1) | (1u << 2);

        if (free==heap->free_h) heap->free_h = free->forward;
        if (prev != NULL) prev->forward = free->forward;

        free->forward = NULL;

        return (char *)free + sizeof(_block_header);
    }
  #endif

    printf("*my_malloc* not enough space after GC...\n");
    printf("*heap used* %ld / %u, requested %u bytes\n",
           heap->top - heap->base,
           heap->size,
           total);

    return NULL;
}
