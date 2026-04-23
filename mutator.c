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

#define  MAX_KEY_VALUE  1000
#define  HEAP_SIZE      (2 * 1024)  /* 2 KByte */

Heap*    heap;      /* shared global variable */
BisTree* roots;     /* shared global variable */

int main(int argc, char** argv) {

   /* initialize the main parameters */
   int threshold  = atoi(argv[1]);  /* an integer in (0,100) */
   int max_roots  = atoi(argv[2]);  /* a positive integer */
   int max_rounds = atoi(argv[3]);  /* a positive integer */   

   /* initialize the heap space used to store tree nodes */
   heap  = (Heap*)malloc(sizeof(Heap));
   heap_init(heap, HEAP_SIZE, mark_sweep_gc);

   /* initialize set of tree roots */
   roots = (BisTree*)malloc(max_roots * sizeof(BisTree));
   for( int i = 0; i < max_roots; i++ )
      bistree_init(&roots[i]);

   /* add/delete integers to/from the trees */
   srandom(getpid());
   for( int i = 0; i < max_rounds; i++ ) {
      /* select bistree */
      BisTree* aroot = &roots[random() % max_roots]; 
      /* toss coin */
      int toss = random() % 100;
      if( toss > threshold ) { 
         /* add integer to one of the roots */
         bistree_insert(aroot, random() % MAX_KEY_VALUE);
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
   /* exit gracefully */
   heap_destroy(heap);
   free(roots);
   return 0;
}
