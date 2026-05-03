/*
 * heap.h
 */

#ifndef HEAP_H
#define HEAP_H

#include "bistree.h"
#include "list.h"

typedef struct {
  void *forward;
  unsigned int marked;
  unsigned int size;
} _block_header;

typedef struct {
  unsigned int size;
  char *base;
  char *top;
  char *limit;
  List *freeb;
  void (*collector)(BisTree *);
} Heap;

void heap_init(Heap *heap, unsigned int size, void (*collector)(BisTree *));

void heap_destroy(Heap *heap);

void *my_malloc(unsigned int nbytes);

#endif
