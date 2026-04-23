/*
 * bistree.h
 */

#ifndef BISTREE_H
#define BISTREE_H

#include "bool.h"

typedef struct BiTreeNode_ {
   int                  data;
   struct BiTreeNode_*  left;
   struct BiTreeNode_*  right;
} BiTreeNode;


typedef struct BiTree_ {
   int                size;
   BiTreeNode*        root;
} BisTree;


void bistree_init(BisTree* tree);

bool bistree_insert(BisTree* tree, int data);

bool bistree_remove(BisTree* tree, int data);

bool bistree_lookup(BisTree* tree, int data);

void bistree_inorder(BisTree* tree);

#define bistree_size(tree)    ((tree)->size)

#endif
