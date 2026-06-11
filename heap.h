#ifndef HEAP_H
#define HEAP_H

#include "bistree.h"
#include "list.h"

#define MAX_FIELDS 8

#define OFFSET(i) (i == 0 ? sizeof(int) : sizeof(char *))

typedef struct {
  unsigned int marked;
  unsigned int size;
  unsigned int n_fields;
  unsigned int field_types;
  void *forward;
} _block_header;

typedef struct {
  unsigned int size;
  char *base;
  char *top;
  char *limit;
  void (*collector)(void *, int);
#ifdef COPY_COLLECT
  char *from;
  char *to;
  _block_header *wip_h; // header for first mem slot in "worklist"
#elif MARK_SWEEP
  _block_header *free_h; // header for first mem slot in "freelist"
#endif
} Heap;

void heap_init(Heap *heap, unsigned int size, void (*collector)(void *, int));

void heap_destroy(Heap *heap);

void *my_malloc(unsigned int nbytes);

#endif
