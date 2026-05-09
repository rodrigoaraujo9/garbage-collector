/*
 * the mutator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "bool.h"
#include "heap.h"
#include "list.h"
#include "bistree.h"
#include "globals.h"
#include "collector.h"

#if \
   (defined(MARK_SWEEP) && defined(MARK_COMPACT)) || \
   (defined(MARK_SWEEP) && defined(COPY_COLLECT)) || \
   (defined(MARK_COMPACT) && defined(COPY_COLLECT))
#error "*error* Define only one GC strategy: MARK_SWEEP, MARK_COMPACT, or COPY_COLLECT"
#endif

#define  MAX_KEY_VALUE  15
#define  HEAP_SIZE      (2 * 1024)  /* 2 KByte */

Heap*    heap;      /* shared global variable */
BisTree* roots;     /* shared global variable */

int main(int argc, char** argv) {

   /* initialize the main parameters */
   int threshold  = atoi(argv[1]);  /* an integer in (0,100) */
   max_roots  = atoi(argv[2]);  /* a positive integer */
   int max_rounds = atoi(argv[3]);  /* a positive integer */

   /* initialize the heap space used to store tree nodes */
   heap  = (Heap*)malloc(sizeof(Heap));
   #if defined(MARK_SWEEP)
      heap_init(heap, HEAP_SIZE, mark_sweep_gc);
   #elif defined(MARK_COMPACT)
      heap_init(heap, HEAP_SIZE, mark_compact_gc);
   #elif defined(COPY_COLLECT)
      heap_init(heap, HEAP_SIZE, copy_collection_gc);
   #else
      #error "*error* you must define one GC strategy: MARK_SWEEP, MARK_COMPACT, or COPY_COLLECT"
   #endif

   /* initialize set of tree roots */
   roots = (BisTree*)malloc(max_roots * sizeof(BisTree));
   for( int i = 0; i < max_roots; i++ )
      bistree_init(&roots[i]);

   /* add/delete integers to/from the trees */
   srandom(getpid());

   /* mesure start time for the program (the main loop) */
   clock_t start = clock();
   gc_ticks = 0;

   for( int i = 0; i < max_rounds; i++ ) {
      /* select bistree */
      BisTree* aroot = &roots[random() % max_roots];
      /* toss coin */
      int toss = random() % 100;
      if( toss > threshold ) {
         /* add integer to one of the roots */
         if (!bistree_insert(aroot, random() % MAX_KEY_VALUE)) break;
         fprintf(stdout, "tree size is %d\n", bistree_size(aroot));
         fprintf(stdout, "(inorder traversal)\n");
         bistree_inorder(aroot);
      }
      else {
         /* remove integer from one of the roots */
         bistree_remove(aroot, random() % MAX_KEY_VALUE);
         fprintf(stdout, "tree size is %d\n", bistree_size(aroot));
         fprintf(stdout, "(inorder traversal)\n");
         bistree_inorder(aroot);
      }
   }

   /* mesure time that program execution took overall (main loop)  */
   clock_t total_ticks = clock() - start;

   /* print fraction of time spent on garbage collection */
   printf("\n%% of time spent on garbage collection: %.2f%%\n", 100.0 * ((double)gc_ticks / (double)total_ticks));

   /* exit gracefully */
   heap_destroy(heap);
   free(roots);
   return 0;
}
