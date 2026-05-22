/*
 * heap.h
 */

#ifndef HEAP_H
#define HEAP_H

#include "bistree.h"
#include "list.h"

typedef struct {
  unsigned int marked;
  unsigned int size;
  unsigned int n_fields;
  unsigned char *field_bitmap; // 0 -> constant value, 1 -> pointer
#if defined(MARK_COMPACT) || defined(COPY_COLLECT)
  void *forward;
#endif
} _block_header;

typedef struct {
  unsigned int size;
  char *base;
  char *top;
  char *limit;
#ifdef MARK_SWEEP
  List *freeb;
#endif
  void (*collector)(BisTree *);
#ifdef COPY_COLLECT
  char *fromSpace;
  char *toSpace;
  List *workList;
#endif
} Heap;

void heap_init(Heap *heap, unsigned int size, void (*collector)(BisTree *));

void heap_destroy(Heap *heap);

void *my_malloc(unsigned int nbytes);

#endif
