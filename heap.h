#ifndef HEAP_H
#define HEAP_H

#include "bistree.h"
#include "list.h"

#define MAX_FIELDS 8

#define OFFSET(i) (i == 0 ? sizeof(int) : sizeof(char *))

/* current architecture doesn't support multiple size of structure without
 * wasting space (no shrimp *nk) */

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
  _block_header *first_freeb_h;
  void (*collector)(void *, int);
#ifdef COPY_COLLECT
  char *from;
  char *to;
  List *wip;
#endif
} Heap;

void heap_init(Heap *heap, unsigned int size, void (*collector)(void *, int));

void heap_destroy(Heap *heap);

void *my_malloc(unsigned int nbytes);

#endif
