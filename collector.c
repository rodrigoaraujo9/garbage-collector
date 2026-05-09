/*
 * collector.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bistree.h"
#include "globals.h"
#include "heap.h"
#include "list.h"

/* Mark */
void mark(BiTreeNode *node);

/* Sweep */
void sweep();

/* Compact */
void compact(BisTree *roots);

/* Collect */
void collect(BisTree *roots);
void *copy(BiTreeNode *fromRef);
void swap();
BiTreeNode *forward(BiTreeNode *fromRef);
void process(BiTreeNode **node);


/* Implementation */

void mark(BiTreeNode *node) {
    if (node == NULL) return;

    _block_header *header = (_block_header *)((char *)node - sizeof(_block_header));

    if (header->marked) return;

    header->marked = true;

    mark(node->left);
    mark(node->right);
}

void sweep() {
    while (!list_isempty(heap->freeb)) {
        list_removefirst(heap->freeb);
    }

    _block_header *current_header = (_block_header *)heap->base;

    while ((char *)current_header < heap->top) {
        _block_header *next_header = (_block_header *)((char *)current_header + sizeof(_block_header) + current_header->size);

        if ((char *)next_header > heap->top) {
            printf("*error* invalid next header");
            return;
        }

        if (!current_header->marked) {
            void *p = (char *)current_header + sizeof(_block_header);
            list_addlast(heap->freeb, p);
        } else {
            current_header->marked = 0;
        }

        current_header = next_header;
    }
}

void compact(BisTree *roots) {
  #ifdef MARK_COMPACT

    /* Cleanup */

    while (!list_isempty(heap->freeb)) {
        list_removefirst(heap->freeb);
    }

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

    for (int i = 0; i < max_roots; i++) {
        BiTreeNode *ref = roots[i].root;
        if (ref != NULL) {
            _block_header *header = (_block_header *)((char *)ref - sizeof(_block_header));
            roots[i].root = (BiTreeNode *)header->forward;
        }
    }

    scan = heap->base;

    while (scan < heap->top) {
        _block_header *header = (_block_header *)scan;
        char *next = scan + sizeof(_block_header) + header->size;
        if (header->marked) {
            BiTreeNode *node = (BiTreeNode *)(scan + sizeof(_block_header));

            if (node->left != NULL) {
                _block_header *header = (_block_header *)((char *)node->left - sizeof(_block_header));
                node->left = (BiTreeNode *)header->forward;
            }

            if (node->right != NULL) {
                _block_header *header = (_block_header *)((char *)node->right - sizeof(_block_header));
                node->right = (BiTreeNode *)header->forward;
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

void collect(BisTree* roots) {
  #ifdef COPY_COLLECT

    /* Cleanup */

    while (!list_isempty(heap->workList)) {
        list_removefirst(heap->workList);
    }

    heap->base = heap->toSpace;
    heap->top = heap->toSpace;
    heap->limit = heap->toSpace + heap->size / 2;

    /* Process Roots */

    for (int i = 0; i < max_roots; i++) {
        process(&roots[i].root);
    }

    /* Process Remaining Nodes */

    while (!list_isempty(heap->workList)) {
        BiTreeNode *ref = list_getfirst(heap->workList);
        list_removefirst(heap->workList);

        process(&ref->left);
        process(&ref->right);
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

void *copy(BiTreeNode *fromRef) {
  #ifdef COPY_COLLECT

    /* Verify */

    if (fromRef == NULL) return NULL;

    _block_header *fromHeader = (_block_header *)((char *)fromRef - sizeof(_block_header));
    unsigned int total = sizeof(_block_header) + fromHeader->size;

    if (heap->top + total > heap->limit) {
        printf("*error* copy collector overflow\n");
        exit(1);
    }

    /* Move */

    char *toHeader = heap->top;
    char *toRef = toHeader + sizeof(_block_header);

    memcpy(toHeader, fromHeader, total);

    /* Cleanup */

    ((_block_header *)toHeader)->forward = NULL;
    ((_block_header *)toHeader)->marked = 0;

    /* Forward */

    fromHeader->forward = toRef;
    heap->top = toHeader + total;

    /* Add to work list */

    list_addlast(heap->workList, toRef);

    return toRef;

  #else
    printf("*error* to use copy() activate COPY_COLLECT");
    exit(1);
  #endif
}

void swap() {
  #ifdef COPY_COLLECT

    char *tmp = heap->toSpace;

    heap->fromSpace = heap->toSpace;
    heap->toSpace = tmp;

  #else
    printf("*error* to use swap() activate COPY_COLLECT");
    exit(1);
  #endif
}

BiTreeNode *forward(BiTreeNode *fromRef) {
  #ifdef COPY_COLLECT

    if (fromRef == NULL) return NULL;

    _block_header *header = (_block_header *)((char *)fromRef - sizeof(_block_header));

    if (header->forward == NULL) {
        return copy(fromRef);
    }

    return (BiTreeNode *)header->forward;

  #else
    printf("*error* to use forward() activate COPY_COLLECT");
    exit(1);
  #endif
}

void process(BiTreeNode **node) {
  #ifdef COPY_COLLECT

    if (*node != NULL) *node = forward(*node);

  #else
    printf("*error* to use process() activate COPY_COLLECT");
    exit(1);
  #endif
}

void mark_sweep_gc(BisTree* roots) {
    printf("*collector* gcing()...\n");

    /* Mark */

    printf("*collector* marking()...\n");
    for (int i = 0; i < max_roots; i++) {
        mark(roots[i].root);
    }

    /* Sweep */

    printf("*collector* sweeping()...\n");
    sweep();

    return;
}

void mark_compact_gc(BisTree* roots) {
    printf("*collector* gcing()...\n");

    /* Mark */

    printf("*collector* marking()...\n");
    for (int i = 0; i < max_roots; i++) {
        mark(roots[i].root);
    }

    /* Compact */

    printf("*collector* compacting()... \n");
    compact(roots);

    return;
}

 void copy_collection_gc(BisTree* roots) {
     printf("*collector* gcing()...\n");

     /* Copy and Collect*/

     printf("*collector* collecting()...\n");
     collect(roots);

     return;
 }
