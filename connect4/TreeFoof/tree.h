#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHILDREN 4
#define MAX_STATE_LENGTH 10

typedef struct Node {
    bool minmax ; // whether it is a minimzer or maximizer 
    char action ; // the place to add a disk 
    struct Node *children[MAX_CHILDREN] ;
    struct Node *parent;
   
} Node;

Node *createNode(char *state, char isRoot);
Node *addChild(Node *parent, char *state);
Node *getParent(Node *child);

void printTree(const Node *node, int depth);
void freeTree(Node *node);
void deleteNChild(Node *node , int N);

#endif