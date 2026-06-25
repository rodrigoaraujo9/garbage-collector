#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "heap.h"
#include "globals.h"
#include "collector.h"
#include "list.h"

void heap_init(Heap* heap, unsigned int size, void (*collector)(void *, int)){
    heap->base  = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    heap->size  = size;
    heap->limit = heap->base + size;
    heap->top   = heap->base;
    heap->collector = collector;

  #ifdef MARK_SWEEP
    for (int i=0; i<MAX_SIZE; i++) {
        list_init(&heap->free[i]);
    }
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

/* Extensions to list to handle free */

void heap_free(void* slot, size_t size) {
 #ifdef MARK_SWEEP
    if (size>MAX_SIZE){
        printf("*error* tried to use an unsupported sized structure in heap_free()\n");
        exit(1);
    }
    if (slot==NULL) return;
    list_addlast(&heap->free[size], slot, size);
    return;
  #else
    printf("*error* to use heap_free() activate MARK_SWEEP");
    exit(1);
  #endif
}

// to use must guard case where returns null
void *heap_alloc(size_t size) {
  #ifdef MARK_SWEEP
    if (size>MAX_SIZE){
        printf("*error* tried to use an unsupported sized structure in heap_pop_free() %d\n", (int)size);
        exit(1);
    }
    void* slot = list_getfirst(&heap->free[size]);
    list_removefirst(&heap->free[size]);
    return slot;
  #else
    printf("*error* to use heap_pop_free() activate MARK_SWEEP");
    exit(1);
  #endif
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
    void* free = heap_alloc(nbytes);
    if (free != NULL) {
        _block_header *free_h = (_block_header *)((char *)free - sizeof(_block_header));
        free_h->marked = 0;

        free_h->n_fields = 3;
        free_h->field_types = (1u << 1) | (1u << 2);

        return free;
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
    free = heap_alloc(nbytes);
    if (free != NULL) {
        _block_header *free_h = (_block_header *)((char *)free - sizeof(_block_header));
        free_h->marked = 0;

        free_h->n_fields = 3;
        free_h->field_types = (1u << 1) | (1u << 2);

        return free;
    }
  #endif

    printf("*my_malloc* not enough space after GC...\n");
    printf("*heap used* %ld / %u, requested %u bytes\n",
           heap->top - heap->base,
           heap->size,
           total);

    return NULL;
}
