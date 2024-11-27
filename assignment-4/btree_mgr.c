#include "btree_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdio.h>
#include <stdlib.h>

// Btree has three type of nodes
// Root, Internal and Leaf
typedef struct treeNode {
    int key;
    int value;
    bool isLeaf;
    struct treeNode *left;
    struct treeNode *right;
} treeNode;

// init and shutdown index manager
RC initIndexManager (void *mgmtData) {
    printf("Index manager was born :)");
    return 0;
}
RC shutdownIndexManager () {
    printf("Index Manager is dead ;(");
    return 0;
}

RC createBtree(char *idxId, DataType keyType, int n) {
    treeNode* result = malloc(sizeof(treeNode));
    if(result != NULL) {

    }
}

RC openBtree(BTreeHandle **tree, char *idxId) {
}

// Close a B+ Tree index
RC closeBtree(BTreeHandle *tree) {
}

// Delete a B+ Tree index
RC deleteBtree(char *idxId) {
}

// Get the number of nodes in the B+ Tree
RC getNumNodes(BTreeHandle *tree, int *result) {
}

// Get the number of entries in the B+ Tree
RC getNumEntries(BTreeHandle *tree, int *result) {
}


// Get the key type of the B+ Tree
RC getKeyType(BTreeHandle *tree, DataType *result) {
}


// Find a key in the B+ Tree
RC findKey(BTreeHandle *tree, Value *key, RID *result) {
}


// Insert key into the B+ Tree
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    printf("--------------------------\n"); // Debugging output
}
RC deleteKey(BTreeHandle *tree, Value *key) {
 }

// Open a scan on the B+ Tree
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
}

// Get the next entry in the scan
RC nextEntry(BT_ScanHandle *handle, RID *result) {
}

// Close the scan on the B+ Tree
RC closeTreeScan(BT_ScanHandle *handle) {
}