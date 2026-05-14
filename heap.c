/*
 * the heap
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "heap.h"
#include "globals.h"
#include "collector.h"


void heap_init(Heap* heap, unsigned int size, void (*collector)(BisTree*)){
    heap->base  = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    heap->size  = size;
    heap->limit = heap->base + size;
    heap->top   = heap->base;

    heap->freeb = (List*)malloc(sizeof(List));
    list_init(heap->freeb);

    heap->collector = collector;

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
        _block_header *q = (_block_header *)heap->top;

        q->marked = 0;
        q->size = nbytes;
      #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        q->forward = NULL;
      #endif

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

    while (!list_isempty(heap->freeb)) {
        void *p = list_getfirst(heap->freeb);
        list_removefirst(heap->freeb);

        _block_header *h = (_block_header *)((char *)p - sizeof(_block_header));

        if (h->size >= nbytes) {
            h->marked = 0;
          #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
            h->forward = NULL;
          #endif
            return p;
        }
    }

    printf("\n\n");
    printf("*my_malloc* not enough space, performing GC...\n");

    clock_t start = clock();
    heap->collector(roots);
    gc_ticks += clock() - start;

    printf("\n\n");

    if (heap->top + total <= heap->limit) {
        _block_header *h = (_block_header *)heap->top;

        h->marked = 0;
        h->size = nbytes;
      #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        h->forward = NULL;
      #endif

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

    while (!list_isempty(heap->freeb)) {
        void *p = list_getfirst(heap->freeb);
        list_removefirst(heap->freeb);

        _block_header *h = (_block_header *)((char *)p - sizeof(_block_header));

        if (h->size >= nbytes) {
            h->marked = 0;
          #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
            h->forward = NULL;
          #endif
            return p;
        }
    }

    printf("*my_malloc* not enough space after GC...\n");
    printf("*heap used* %ld / %u, requested %u bytes\n", heap->top - heap->base, heap->size, total);

    return NULL;
}
