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
    heap->base  = mmap ( NULL, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
    heap->size  = size;
    heap->limit = heap->base + size;
    heap->top   = heap->base;
    heap->freeb = (List*)malloc(sizeof(List));
    list_init(heap->freeb);
    heap->collector = collector;
    return;
}

void heap_destroy(Heap* heap) {
    munmap(heap->base, heap->size);
    return;
}

void* my_malloc(unsigned int nbytes) {
    if (heap->top + sizeof(_block_header) + nbytes < heap->limit) {
        _block_header* q = (_block_header*)(heap->top);
        q->marked = 0;
        q->size = nbytes;
        q->forward = NULL;

        char *p = heap->top + sizeof(_block_header);
        heap->top = heap->top + sizeof(_block_header) + nbytes;
        return p;
    }

    printf("my_malloc: not enough space, performing GC...\n");

    clock_t start = clock();
    heap->collector(roots);
    gc_ticks += clock() - start;

    if (heap->top + sizeof(_block_header) + nbytes < heap->limit) {
        _block_header *q = (_block_header *)(heap->top);
        q->marked = 0;
        q->size = nbytes;
        q->forward = NULL;

        char *p = heap->top + sizeof(_block_header);
        heap->top = heap->top + sizeof(_block_header) + nbytes;
        return p;
    }

    printf("my_malloc: not enough space after GC...\n");
    return NULL;
}

void* my_malloc_sweep(unsigned int nbytes) {
    if (heap->top + sizeof(_block_header) + nbytes < heap->limit) {
        _block_header* h = (_block_header*)heap->top;
        h->marked = 0;
        h->size   = nbytes;
        h->forward = NULL;

        char* p = heap->top + sizeof(_block_header);
        heap->top += sizeof(_block_header) + nbytes;
        return p;
    }

    printf("my_malloc: not enough space, performing GC...\n");

    clock_t start = clock();
    heap->collector(roots);
    gc_ticks += clock() - start;

    if (heap->top + sizeof(_block_header) + nbytes < heap->limit) {
        _block_header* h = (_block_header*)heap->top;
        h->marked = 0;
        h->size   = nbytes;
        h->forward = NULL;

        char* p = heap->top + sizeof(_block_header);
        heap->top += sizeof(_block_header) + nbytes;
        return p;
    }

    if (!list_isempty(heap->freeb)) {
        void* p = list_getfirst(heap->freeb);
        list_removefirst(heap->freeb);
        _block_header* h = (_block_header*)((char*)p - sizeof(_block_header));
        if (h->size >= nbytes) {
            h->marked = 0;
            return p;
        }
        return my_malloc_sweep(nbytes);
    }

    printf("my_malloc: not enough space after GC...\n");
    return NULL;
}
