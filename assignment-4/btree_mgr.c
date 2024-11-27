#include "btree_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct treenode {
    struct treenode *left;
    struct treenode *right;
    struct treenode *parent;
    bool is_leaf;
    int count;
};

typedef struct BTreeMgmtData {
    int minDegree;
    int numNodes;           // Total number of nodes in the tree
    int numEntries;         // Total number of entries in the tree
    struct treenode *root;        // Pointer to the root node
} BTreeMgmtData;

RC initIndexManager(void *mgmtData) {
    printf("Index Manager was born\n");
    return RC_OK;
}

// Shutdown the B+ Tree index manager
RC shutdownIndexManager(void *mgmtData) {
    printf("Index Manager was killed\n");
    return RC_OK;
}

RC createBtree(char *idxId, DataType keyType, int n) {
    BTreeHandle *btree = (BTreeHandle *)malloc(sizeof(BTreeHandle));

    if (btree == NULL) {
        printf("B+tree Memory allocation error\n");
    }
    return RC_OK;
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


    return RC_OK;
}

// Get the number of entries in the B+ Tree
RC getNumEntries(BTreeHandle *tree, int *result) {


    return RC_OK;
}

// Get the key type of the B+ Tree
RC getKeyType(BTreeHandle *tree, DataType *result) {
    *result = tree->keyType;

    return RC_OK;
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

    return RC_OK;
}