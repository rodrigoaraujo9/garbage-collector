/*
 * heap.h
 */

#ifndef HEAP_H
#define HEAP_H

#include "bistree.h"
#include "list.h"

#define MAX_FIELDS 8

#define OFFSET(i) ((i == 0 ? sizeof(char)  : sizeof(char *))

typedef struct __attribute__((packed)) {
  unsigned int marked;
  unsigned int size;
  unsigned int n_fields;
  unsigned int field_types; // bitwise each bit represents a field type in order
  void *forward;
} _block_header;

typedef struct {
  unsigned int size;
  char *base;
  char *top;
  char *limit;
  _block_header
      *first_freeb_h; // points to the first free space -> it is a blockheader
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
