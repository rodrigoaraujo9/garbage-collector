/*
 * heap.h
 */

#ifndef HEAP_H
#define HEAP_H

#include "bistree.h"
#include "list.h"

#define MAX_FIELDS 8

typedef struct {
  unsigned int marked;
  unsigned int size;
  unsigned int n_fields;
  unsigned int field_offsets[MAX_FIELDS];
  unsigned int field_types; // bitwise each bit represents a field type in order
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
  void (*collector)(void *, int);
#ifdef COPY_COLLECT
  char *fromSpace;
  char *toSpace;
  List *workList;
#endif
} Heap;

void heap_init(Heap *heap, unsigned int size, void (*collector)(void *, int));

void heap_destroy(Heap *heap);

void *my_malloc(unsigned int nbytes);

#endif
