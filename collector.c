/*
 * collector.c
 */

#include <stdio.h>
#include <string.h>

#include "bistree.h"
#include "globals.h"
#include "heap.h"
#include "list.h"

void mark(BiTreeNode *root) {
    if (root == NULL) return;

    _block_header *header = (_block_header *)((char *)root - sizeof(_block_header));

    if (header->marked)
        return;

    header->marked = true;

    mark(root->left);
    mark(root->right);
}

void mark_sweep_gc(BisTree* roots) {
   printf("marking()...\n");
   for (int i = 0; i < max_roots; i++) {
       mark(roots[i].root);
   }

   printf("sweeping()...\n");

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

   /*
    * compact phase:
    * go through entire heap,
    * compute new addresses
    * copy objects to new addresses
    */
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
