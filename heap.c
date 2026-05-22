/*
 * the heap
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "heap.h"
#include "globals.h"
#include "collector.h"


void heap_init(Heap* heap, unsigned int size, void (*collector)(void *, int)){
    heap->base  = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    heap->size  = size;
    heap->limit = heap->base + size;
    heap->top   = heap->base;
    heap->collector = collector;

  #ifdef MARK_SWEEP
    heap->freeb = (List*)malloc(sizeof(List));
    list_init(heap->freeb);
  #endif

  #ifdef COPY_COLLECT
    heap->toSpace = heap->base;
    heap->fromSpace = heap->base + heap->size / 2;

    heap->workList = (List*)malloc(sizeof(List));
    list_init(heap->workList);

    heap->base = heap->fromSpace;
    heap->top = heap->fromSpace;
    heap->limit = heap->fromSpace + heap->size / 2;
  #endif

    return;
}

void heap_destroy(Heap* heap) {
    munmap(heap->base, heap->size);
    return;
}

void* my_malloc(unsigned int nbytes) {
    unsigned int total = sizeof(_block_header) + nbytes;

    if (heap->top + total <= heap->limit) {
        _block_header *h = (_block_header *)heap->top;

        h->marked = 0;
        h->size = nbytes;
        h->n_fields = 0;

        for (int i = 0; i < MAX_FIELDS; i++)
            h->field_offsets[i] = 0;

      #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        h->forward = NULL;
      #endif

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

  #ifdef MARK_SWEEP
    while (!list_isempty(heap->freeb)) {
        int i;
        void *p = list_getfirstbigger(heap->freeb, nbytes, &i);

        if (p == NULL)
            break;

        list_remove(heap->freeb, i);

        _block_header *h = (_block_header *)((char *)p - sizeof(_block_header));

        h->marked = 0;
        h->size = nbytes;
        h->n_fields = 0;

        for (int j = 0; j < MAX_FIELDS; j++)
            h->field_offsets[j] = 0;

  #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        h->forward = NULL;
  #endif

        return p;
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
        h->n_fields = 0;

        for (int i = 0; i < MAX_FIELDS; i++)
            h->field_offsets[i] = 0;

  #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        h->forward = NULL;
  #endif

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

  #ifdef MARK_SWEEP
    while (!list_isempty(heap->freeb)) {
        int i;
        void *p = list_getfirstbigger(heap->freeb, nbytes, &i);

        if (p == NULL)
            break;

        list_remove(heap->freeb, i);

        _block_header *h = (_block_header *)((char *)p - sizeof(_block_header));

        h->marked = 0;
        h->size = nbytes;
        h->n_fields = 0;

        for (int j = 0; j < MAX_FIELDS; j++)
            h->field_offsets[j] = 0;

  #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        h->forward = NULL;
  #endif

        return p;
    }
  #endif

    printf("*my_malloc* not enough space after GC...\n");
    printf("*heap used* %ld / %u, requested %u bytes\n",
           heap->top - heap->base,
           heap->size,
           total);

    return NULL;
}
