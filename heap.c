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
    if( heap->top + sizeof(_block_header) + nbytes < heap->limit ) {
       _block_header* q = (_block_header*)(heap->top);
       q->marked = 0;
       q->size   = nbytes;
       char *p = heap->top + sizeof(_block_header);
       heap->top = heap->top + sizeof(_block_header) + nbytes;
       return p;
     } 
     else {
       printf("my_malloc: not enough space, performing GC...");
       heap->collector(roots);
       if ( list_isempty(heap->freeb) ) {
          printf("my_malloc: not enough space after GC...");
          return NULL;
       }
       return list_getfirst(heap->freeb);
    }
}
