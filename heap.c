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
    heap->free_h = NULL;
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
