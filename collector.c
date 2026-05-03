/*
 * collector.c
 */

#include <stdio.h>
#include <string.h>

#include "bistree.h"
#include "globals.h"
#include "heap.h"
#include "list.h"

void updateReferences(BiTreeNode *node){
    if (node == NULL) return;

    if (node->left != NULL) {
        _block_header *header = (_block_header *)((char *)node->left - sizeof(_block_header));
        node->left = header->forward;
    }

    if (node->right != NULL) {
        _block_header *header = (_block_header *)((char *)node->right - sizeof(_block_header));
        node->right = header->forward;
    }
}

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
            void *payload = (char *)current_header + sizeof(_block_header);
            list_addlast(heap->freeb, payload);
        } else {
            current_header->marked = 0;
        }

        current_header = next_header;
    }
}

void compact(BisTree *roots) {

    /* Cleanup previous garbage collection */

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
        }

        scan = next;
    }

    char *top = dest;


    /* Update References */

    for (int i = 0; i < max_roots; i++) {
        BiTreeNode *ref = roots[i].root;
        if (ref != NULL) {
            _block_header *header = (_block_header *)((char *)ref - sizeof(_block_header));
            roots[i].root = header->forward;
        }
    }

    scan = heap->base;

    while (scan < heap->top) {
        _block_header *header = (_block_header *)scan;
        char *next = scan + sizeof(_block_header) + header->size;
        if (header->marked) {
            BiTreeNode *root = (BiTreeNode *)(scan + sizeof(_block_header));
            updateReferences(root);
        }
        scan = next;
    }

    /* Relocate */

    scan = heap->base;

    while (scan < heap->top) {
        _block_header *header = (_block_header *)scan;
        char *next = scan + sizeof(_block_header) + header->size;
        if (header->marked) {
            dest = header->forward;
            _block_header *dest_header = (_block_header *)(dest - sizeof(_block_header));
            header->marked=false;
            memmove(dest_header, header, sizeof(_block_header) + header->size);
        }
        scan = next;
    }

    heap->top = top;

}


void mark_sweep_gc(BisTree* roots) {

   /*
    * mark phase
    * - every object has live bit set to 0
    * - start from GC roots
    * - traverse graph, set bit to 1 for every object visited
    */

    printf("marking()...\n");
    for (int i = 0; i < max_roots; i++) {
        mark(roots[i].root);
    }

   /*
    * sweep phase:
    * go through entire heap,
    * add unmarked to free list
    */

    printf("sweeping()...\n");
    sweep();

    printf("gcing()...\n");
    return;
}

void mark_compact_gc(BisTree* roots) {

   /*
    * mark phase:
    * go throught all roots,
    * traverse trees,
    * mark reachable
    */

    printf("marking()...\n");
    for (int i = 0; i < max_roots; i++) {
        mark(roots[i].root);
    }

   /*
    * compact phase:
    * go through entire heap,
    * compute new addresses
    * copy objects to new addresses
    */

   printf("compacting()... \n");
   compact(roots);

   printf("gcing()...\n");
   return;
 }

void copy_collection_gc(BisTree* roots) {
   /*
    * go throught all roots,
    * traverse trees in from_space,
    * copy reachable to to_space
    */
    printf("gcing()...\n");
    return;
}
