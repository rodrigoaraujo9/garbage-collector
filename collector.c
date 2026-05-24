/*
 * collector.c
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bistree.h"
#include "globals.h"
#include "heap.h"
#include "list.h"

/* Mark */
void mark(char *object);

/* Sweep */
void sweep();

/* Compact */
void compact(void *objects, int n_objects);

/* Collect */
void collect(void *objects, int n_objects);
void *copy(void *from);
void swap();
void *forward(void *fromRef);
void process(void **node);

/* ---------------------------------------------------------------------------------------------------------------------------- */

/* Mark */

void mark(char *object) {
    if (object == NULL) return;

    _block_header *header = (_block_header *)((char *)object - sizeof(_block_header));

    if (header->marked) return;

    header->marked = 1;

    for (int i = 0; i < header->n_fields; i++) {
        // create mask for bit i and AND with field_types
        // if result is 0 => bit was 0.. so not a pointer
        if (!(header->field_types & (1u << i)))
            continue;

        void **field = (void **)((char *)object + header->field_offsets[i]);

        if (*field != NULL)
            mark((char *)(*field));
    }
}

/* ---------------------------------------------------------------------------------------------------------------------------- */

/* Sweep */

void sweep() {
  #ifdef MARK_SWEEP
    heap->first_freeb_h = NULL;

    _block_header *current_header = (_block_header *)heap->base;

    while ((char *)current_header < heap->top) {
        _block_header *next_header = (_block_header *)((char *)current_header + sizeof(_block_header) + current_header->size);

        if ((char *)next_header > heap->top) {
            printf("*error* invalid next header");
            return;
        }

        if (!current_header->marked) {
            // iterate from freeb until free block bigger than our one. change foward pointers so it fits in middle.
            // if gets to end point to null and last that was there point to it.
            _block_header *free = heap->first_freeb_h;
            while (free != NULL && free->size < current_header->size) {
                free = free->forward;
            }
            if (free == heap->first_freeb_h) {
                current_header->forward = heap->first_freeb_h;
                heap->first_freeb_h = current_header;
            } else {
                _block_header *prev = heap->first_freeb_h;
                while (prev->forward != free) {
                    prev = prev->forward;
                }
                prev->forward = current_header;
                current_header->forward = free;
            }
        } else {
            current_header->marked = 0;
        }

        current_header = next_header;
    }
  #else
    printf("*error* to use sweep() activate MARK_COMPACT");
    exit(1);
  #endif
}

void mark_sweep_gc(void* objects, int n_objects) {
    printf("*collector* gcing()...\n");

    void **roots = (void **)objects;

    /* Mark */

    printf("*collector* marking()...\n");
    for (int i = 0; i < n_objects; i++) {
        mark((char *)roots[i]);
    }

    /* Sweep */

    printf("*collector* sweeping()...\n");
    sweep();

    return;
}

/* ---------------------------------------------------------------------------------------------------------------------------- */

/* Compact */

void compact(void *objects, int n_objects) {
  #ifdef MARK_COMPACT

    /* Compute Locations */

    char *scan = heap->base;
    char *dest = heap->base;

    while (scan < heap->top) {
        _block_header *header = (_block_header *)scan;
        char *next = scan + sizeof(_block_header) + header->size;

        if (header->marked) {
            header->forward = dest + sizeof(_block_header);
            dest += sizeof(_block_header) + header->size;
        } else {
            header->forward = NULL;
        }

        scan = next;
    }

    char *top = dest;

    /* Update References */

    void **roots = (void **)objects;

    for (int i = 0; i < n_objects; i++) {
        void *object = roots[i];

        if (object != NULL) {
            _block_header *header = (_block_header *)((char *)object - sizeof(_block_header));

            roots[i] = header->forward;
        }
    }
    scan = heap->base;

    while (scan < heap->top) {
        _block_header *header = (_block_header *)scan;
        char *next = scan + sizeof(_block_header) + header->size;

        if (header->marked) {
            char *object = scan + sizeof(_block_header);

            for (int i = 0; i < header->n_fields; i++) {
                if (!(header->field_types & (1u << i)))
                    continue;

                void **field = (void **)(object + header->field_offsets[i]);

                if (*field != NULL) {
                    _block_header *field_header =
                        (_block_header *)((char *)(*field) - sizeof(_block_header));

                    *field = field_header->forward;
                }
            }
        }

        scan = next;
    }

    /* Relocate */

    scan = heap->base;

    while (scan < heap->top) {
        _block_header *header = (_block_header *)scan;
        unsigned int size = header->size;
        char *next = scan + sizeof(_block_header) + size;

        if (header->marked) {
            char *dest = (char *)header->forward;
            char *dest_header = dest - sizeof(_block_header);

            memcpy(dest_header, scan, sizeof(_block_header) + size);

            _block_header *new_header = (_block_header *)dest_header;
            new_header->marked = 0;
            new_header->forward = NULL;
        }

        scan = next;
    }

    heap->top = top;

    printf("*heap used* %ld / %u\n", heap->top - heap->base, heap->size);
  #else
    printf("*error* to use compact() activate MARK_COMPACT");
    exit(1);
  #endif

}

void mark_compact_gc(void* objects, int n_objects) {
    printf("*collector* gcing()...\n");

    void **roots = (void **)objects;

    /* Mark */

    printf("*collector* marking()...\n");
    for (int i = 0; i < n_objects; i++) {
        mark((char *)roots[i]);
    }

    /* Compact */

    printf("*collector* compacting()... \n");
    compact(objects, n_objects);
}

/* ---------------------------------------------------------------------------------------------------------------------------- */

/* Copy Collection */

void collect(void *objects, int n_objects) {
  #ifdef COPY_COLLECT

    /* Cleanup */

    while (!list_isempty(heap->workList)) {
        list_removefirst(heap->workList);
    }

    heap->base = heap->toSpace;
    heap->top = heap->toSpace;
    heap->limit = heap->toSpace + heap->size / 2;

    /* Process Roots */

    void **roots = (void **)objects;

    for (int i = 0; i < n_objects; i++) {
        process(&roots[i]);
    }

    /* Process Remaining Nodes */

    while (!list_isempty(heap->workList)) {
        void *object = list_getfirst(heap->workList);
        list_removefirst(heap->workList);

        _block_header *header = (_block_header *)((char *)object - sizeof(_block_header));

        for (int i = 0; i < header->n_fields; i++) {
            if (!(header->field_types & (1u << i)))
                continue;

            void **field = (void **)((char *)object + header->field_offsets[i]);

            process(field);
        }
    }

    /* Cleanup */

    char *tmp = heap->fromSpace;
    heap->fromSpace = heap->toSpace;
    heap->toSpace = tmp;

  #else
    printf("*error* to use collect() activate COPY_COLLECT");
    exit(1);
  #endif
}

void *copy(void *from) {
  #ifdef COPY_COLLECT

    /* Verify */

    if (from == NULL) return NULL;

    _block_header *fromHeader = (_block_header *)((char *)from - sizeof(_block_header));
    unsigned int size = sizeof(_block_header) + fromHeader->size;

    if (heap->top + size > heap->limit) {
        printf("*error* copy collector overflow\n");
        exit(1);
    }

    /* Move */

    char *toHeader = heap->top;
    char *to = toHeader + sizeof(_block_header);

    memcpy(toHeader, fromHeader, size);

    /* Cleanup */

    ((_block_header *)toHeader)->forward = NULL;
    ((_block_header *)toHeader)->marked = 0;

    /* Forward */

    fromHeader->forward = to;
    heap->top = toHeader + size;

    /* Add to work list */

    list_addlast(heap->workList, to, fromHeader->size);

    return to;

  #else
    printf("*error* to use copy() activate COPY_COLLECT");
    exit(1);
  #endif
}

void swap() {
  #ifdef COPY_COLLECT

    char *tmp = heap->fromSpace;

    heap->fromSpace = heap->toSpace;
    heap->toSpace = tmp;

  #else
    printf("*error* to use swap() activate COPY_COLLECT");
    exit(1);
  #endif
}

void *forward(void *node) {
  #ifdef COPY_COLLECT

    if (node == NULL) return NULL;

    _block_header *header = (_block_header *)((char *)node - sizeof(_block_header));

    if (header->forward == NULL) {
        return copy(node);
    }

    return (void *)header->forward;

  #else
    printf("*error* to use forward() activate COPY_COLLECT");
    exit(1);
  #endif
}

void process(void **node) {
  #ifdef COPY_COLLECT

    if (*node != NULL) *node = forward(*node);

  #else
    printf("*error* to use process() activate COPY_COLLECT");
    exit(1);
  #endif
}

void copy_collection_gc(void *objects, int n_objects) {
    printf("*collector* gcing()...\n");

    /* Copy and Collect*/

    printf("*collector* collecting()...\n");
    collect(objects, n_objects);

     return;
}
