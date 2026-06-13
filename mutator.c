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
#define  HEAP_SIZE      (2 * 1024)

Heap*    heap;
BisTree* roots;

int main(int argc, char** argv) {

   int threshold  = atoi(argv[1]);  // an integer in (0,100)
   max_roots  = atoi(argv[2]);      // a positive integer
   int max_rounds = atoi(argv[3]);  // a positive integer

   heap  = (Heap*)malloc(sizeof(Heap));
   #if defined(MARK_SWEEP)
      heap_init(heap, HEAP_SIZE, mark_sweep_gc);
   #elif defined(MARK_COMPACT)
      heap_init(heap, HEAP_SIZE, mark_compact_gc);
   #elif defined(COPY_COLLECT)
      heap_init(heap, HEAP_SIZE, copy_collection_gc);
   #else
      #error "*error*     you must define one GC strategy: MARK_SWEEP, MARK_COMPACT, or COPY_COLLECT"
   #endif

   roots = (BisTree*)malloc(max_roots * sizeof(BisTree));
   for( int i = 0; i < max_roots; i++ )
      bistree_init(&roots[i]);

   srandom(getpid());

   clock_t start = clock();
   gc_ticks = 0;

   for( int i = 0; i < max_rounds; i++ ) {
      BisTree* aroot = &roots[random() % max_roots];
      int toss = random() % 100;
      if( toss > threshold ) {
         if (!bistree_insert(aroot, random() % MAX_KEY_VALUE)) break;
         fprintf(stdout, "tree size is %d\n", bistree_size(aroot));
         fprintf(stdout, "(inorder traversal)\n");
         bistree_inorder(aroot);
      }
      else {
         bistree_remove(aroot, random() % MAX_KEY_VALUE);
         fprintf(stdout, "tree size is %d\n", bistree_size(aroot));
         fprintf(stdout, "(inorder traversal)\n");
         bistree_inorder(aroot);
      }
   }

   clock_t total_ticks = clock() - start;

   printf("\n*info* %% of time spent on garbage collection: %.2f%%\n", 100.0 * ((double)gc_ticks / (double)total_ticks));

   heap_destroy(heap);
   free(roots);
   return 0;
}
