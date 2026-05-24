/*
 * bistree.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "bool.h"
#include "bistree.h"
#include "heap.h"

void bistree_init(BisTree* tree) {
    tree->root = NULL;
    tree->size = 0;
    return;
}

bool bitreenode_lookup(BiTreeNode* node, int data) {
   if (node == NULL)
      return false;
   if (data < node->data)
      return bitreenode_lookup(node->left, data);
   if (data > node->data)
      return bitreenode_lookup(node->right, data);
   return true;
}

bool bistree_lookup(BisTree* tree, int data) {
    return bitreenode_lookup(tree->root, data);
}

BiTreeNode* bitreenode_insert(BiTreeNode* node, int data, BiTreeNode* new_node) {
    if (node == NULL) {
        BiTreeNode* node = new_node;
        node->data = data;
        node->left = NULL;
        node->right= NULL;
        return node;
     }

    if (new_node->data < node->data)
        node->left = bitreenode_insert(node->left, data, new_node);
    else
        node->right = bitreenode_insert(node->right, data, new_node);

    return node;
}

bool bistree_insert(BisTree* tree, int data) {
    BiTreeNode *node = (BiTreeNode *)my_malloc(sizeof(BiTreeNode));

    if (bistree_lookup(tree, data))
        return true;

    if (node == NULL)
        return false;

    _block_header *h = (_block_header *)((char *)node - sizeof(_block_header));

    h->n_fields = 3;

    h->field_offsets[0] = offsetof(BiTreeNode, left);
    h->field_types |= (1u << 0);

    h->field_offsets[1] = offsetof(BiTreeNode, right);
    h->field_types |= (1u << 1);

    h->field_offsets[2] = offsetof(BiTreeNode, data);
    h->field_types &= ~(1u << 2);

    node->data = data;
    node->left = NULL;
    node->right = NULL;

    tree->root = bitreenode_insert(tree->root, data, node);
    tree->size++;

    return true;
}

BiTreeNode* bitreenode_remove(BiTreeNode* node, int data) {
    if (node == NULL)
        return NULL;
    if(data < node->data)
        node->left = bitreenode_remove(node->left, data);
    else if(data > node->data)
        node->right = bitreenode_remove(node->right, data);
    else if(node->left == NULL)
        node = node->right;
    else if(node->right == NULL)
        node = node->left;
    else {
        BiTreeNode* lnode = node->left;
        while(lnode->right != NULL)
            lnode = lnode->right;
        node->data = lnode->data;
        node->left = bitreenode_remove(node->left, lnode->data);
   }
   return node;
}

bool bistree_remove(BisTree* tree, int data) {
   if (tree == NULL) return false;
   if (!bistree_lookup(tree, data))
      return false;
   tree->root = bitreenode_remove(tree->root, data);
   tree->size = tree->size - 1;
   return true;
}

void bitreenode_inorder(BiTreeNode* node) {
   if(node == NULL)
      return;
   bitreenode_inorder(node->left);
   printf(" %d ", node->data);
   bitreenode_inorder(node->right);
}

void bistree_inorder(BisTree* tree) {
   printf("root: %p\n", tree->root);
   bitreenode_inorder(tree->root);
   printf("\n");
}
