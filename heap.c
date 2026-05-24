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
    heap->first_freeb_h = NULL;
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

        h->n_fields = 3;
        h->field_types = 0;

        h->field_offsets[0] = offsetof(BiTreeNode, data);
        h->field_offsets[1] = offsetof(BiTreeNode, left);
        h->field_offsets[2] = offsetof(BiTreeNode, right);

        h->field_types = (1u << 1) | (1u << 2); // 000...110 -> 1 is pointer and 0 is constant

      #if defined(MARK_COMPACT) || defined(COPY_COLLECT)
        h->forward = NULL;
      #endif

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

  #ifdef MARK_SWEEP
    _block_header *free = heap->first_freeb_h;
    _block_header *prev = NULL;
    while (free!=NULL && free->size < nbytes) {
        // iterate from freeb until the first free block that accomodates the nbytes
        prev = free;
        free = free->forward;
    }

    if (free != NULL) {
        free->marked = 0;
        // free->size = nbytes;   // no shrink logic yet -> only when it propperly splits
        free->n_fields = 3;
        free->field_types = 0;

        free->field_offsets[0] = offsetof(BiTreeNode, data);
        free->field_offsets[1] = offsetof(BiTreeNode, left);
        free->field_offsets[2] = offsetof(BiTreeNode, right);

        free->field_types = (1u << 1) | (1u << 2); // 000...110 -> 1 is pointer and 0 is constant

        if (free==heap->first_freeb_h) heap->first_freeb_h = free->forward;
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
        h->field_types = 0;

        h->field_offsets[0] = offsetof(BiTreeNode, data);
        h->field_offsets[1] = offsetof(BiTreeNode, left);
        h->field_offsets[2] = offsetof(BiTreeNode, right);

        h->field_types = (1u << 1) | (1u << 2); // 000...110 -> 1 is pointer and 0 is constant

        h->forward = NULL;

        void *p = heap->top + sizeof(_block_header);
        heap->top += total;

        return p;
    }

  #ifdef MARK_SWEEP
    free = heap->first_freeb_h;
    prev = NULL;
    while (free!=NULL && free->size < nbytes) {
        // iterate from freeb until the first free block that accomodates the nbytes
        prev = free;
        free = free->forward;
    }

    if (free != NULL) {
        free->marked = 0;
        // free->size = nbytes;   // no shrink logic yet -> only when it propperly splits
        free->n_fields = 3;
        free->field_types = 0;

        free->field_offsets[0] = offsetof(BiTreeNode, data);
        free->field_offsets[1] = offsetof(BiTreeNode, left);
        free->field_offsets[2] = offsetof(BiTreeNode, right);

        free->field_types = (1u << 1) | (1u << 2); // 000...110 -> 1 is pointer and 0 is constant

        if (free==heap->first_freeb_h) heap->first_freeb_h = free->forward;
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
