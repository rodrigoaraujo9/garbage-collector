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
void *forward(void *node);
void process(void **node);

/* ---------------------------------------------------------------------------------------------------------------------------- */

/* Mark */

void mark(char *object) {
    if (object == NULL) return;

    _block_header *header = (_block_header *)((char *)object - sizeof(_block_header));

    if (header->marked) return;

    header->marked = 1;

    char *offset = (char *)object;

    for (int i = 0; i < header->n_fields; i++) {
        bool is_pointer = (header->field_types & (1u << i));

        if (!is_pointer){
            offset += OFFSET(is_pointer);
            continue;
        }

        void **field = (void **)offset;
        offset += OFFSET(is_pointer);

        if (*field != NULL)
            mark((char *)(*field));
    }
}

/* ---------------------------------------------------------------------------------------------------------------------------- */

/* Sweep */

void sweep() {
  #ifdef MARK_SWEEP
    heap->free_h = NULL;

    _block_header *current_header = (_block_header *)heap->base;

    while ((char *)current_header < heap->top) {
        _block_header *next_header = (_block_header *)((char *)current_header + sizeof(_block_header) + current_header->size);

        if ((char *)next_header > heap->top) {
            printf("*error* invalid next header");
            return;
        }

        if (!current_header->marked) {
            _block_header *free = heap->free_h;
            while (free != NULL && free->size < current_header->size) {
                free = free->forward;
            }
            if (free == heap->free_h) {
                current_header->forward = NULL;
                heap->free_h = current_header;
            } else {
                _block_header *prev = heap->free_h;
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
    printf("*error* to use sweep() activate MARK_SWEEP");
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

    char *needle = heap->base;
    char *dest   = heap->base;

    while (needle < heap->top) {
        _block_header *header = (_block_header *)needle;
        char *next = needle + sizeof(_block_header) + header->size;

        if (header->marked) {
            header->forward = dest + sizeof(_block_header);
            dest += sizeof(_block_header) + header->size;
        } else {
            header->forward = NULL;
        }

        needle = next;
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
    needle = heap->base;

    while (needle < heap->top) {
        _block_header *header = (_block_header *)needle;
        char *next = needle + sizeof(_block_header) + header->size;

        if (header->marked) {
            char *object = needle + sizeof(_block_header);

            char *offset = (char *)object;

            for (int i = 0; i < header->n_fields; i++) {
                bool is_pointer = (header->field_types & (1u << i));

                if (!is_pointer){
                    offset += OFFSET(is_pointer);
                    continue;
                }

                void **field = (void **)offset;
                offset += OFFSET(is_pointer);

                if (*field != NULL) {
                    _block_header *field_header = (_block_header *)((char *)(*field) - sizeof(_block_header));
                    *field = field_header->forward;
                }
            }
        }

        needle = next;
    }

    /* Relocate */

    needle = heap->base;

    while (needle < heap->top) {
        _block_header *header = (_block_header *)needle;
        unsigned int size = header->size;
        char *next = needle + sizeof(_block_header) + size;

        if (header->marked) {
            char *dest = (char *)header->forward;
            char *dest_header = dest - sizeof(_block_header);

            memcpy(dest_header, needle, sizeof(_block_header) + size);

            _block_header *new_header = (_block_header *)dest_header;
            new_header->marked = 0;
            new_header->forward = NULL;
        }

        needle = next;
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

/* Cheney-style Copy Collection */

void collect(void *objects, int n_objects) {
  #ifdef COPY_COLLECT

    /* Cleanup */

    heap->wip_h = NULL;

    heap->top   = heap->to;
    heap->limit = heap->to + heap->size / 2;

    /* Process Roots */

    void **roots = (void **)objects;

    for (int i = 0; i < n_objects; i++) {
        process(&roots[i]);
    }

    /* Process Remaining Nodes */

    while (heap->wip_h != NULL) {
        _block_header *header = heap->wip_h;
        _block_header *next = (_block_header *)header->forward;

        heap->wip_h = next;
        header->forward = NULL;

        char *object = (char *)header + sizeof(_block_header);
        char *offset = object;

        for (int i = 0; i < header->n_fields; i++) {
            bool is_pointer = header->field_types & (1u << i);

            if (is_pointer) {
                void **field = (void **)offset;
                process(field);
            }

        offset += OFFSET(is_pointer);
        }
    }

    /* Cleanup */

    char *tmp  = heap->from;
    heap->from = heap->to;
    heap->to   = tmp;

  #else
    printf("*error* to use collect() activate COPY_COLLECT");
    exit(1);
  #endif
}

void *copy(void *from) {
  #ifdef COPY_COLLECT

    /* Verify */

    if (from == NULL) return NULL;

    _block_header *from_header = (_block_header *)((char *)from - sizeof(_block_header));
    unsigned int size = sizeof(_block_header) + from_header->size;

    if (heap->top + size > heap->limit) {
        printf("*error* copy collector overflow\n");
        exit(1);
    }

    /* Move */

    _block_header *to_header = (_block_header *) heap->top;
    char *to = (char *)to_header + sizeof(_block_header);

    memcpy(to_header, from_header, size);

    /* Cleanup */

    to_header->forward = NULL;
    to_header->marked  = 0;

    /* Forward */

    from_header->forward = to;
    heap->top = (char *)to_header + size;

    /* Add to "worklist" */

    to_header->forward = NULL;

    if (heap->wip_h == NULL) {
        heap->wip_h = to_header;
    } else {
        _block_header *needle = heap->wip_h;

        while (needle->forward != NULL) {
            needle = (_block_header *)needle->forward;
        }

        needle->forward = to_header;
    }

    return to;

  #else
    printf("*error* to use copy() activate COPY_COLLECT");
    exit(1);
  #endif
}

void swap() {
  #ifdef COPY_COLLECT

    char *tmp  = heap->from;
    heap->from = heap->to;
    heap->to   = tmp;

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
