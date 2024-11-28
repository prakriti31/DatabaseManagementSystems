#include "btree_mgr.h"

#include <ctype.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct node {
    int * keys; // Array of keys
    void ** ptrs; // Array of pointers. Corresponding to keys with same index
    int num_keys; // Number of keys currently present in the node
    bool is_leaf; // Is the node an internal node or a leaf node?
    struct node *next_leaf; // Pointer to the next leaf node, if this node is indeed a leaf node
    struct node *parent; // Parent node for this node, NULL if no parent (Root)
    bool is_root;
    int max_keys_per_node;
    int max_ptrs_per_node;
} node;

typedef struct metadata {
    int order; // Maximum number of keys allowed for a node
    int nodes; // Count for total existing nodes in the tree
    int entries; // Total number of entries in the index / tree
    DataType type; // Datatype of the key, Default is DT_INT, extra implementation?
    node * root; // Root of the tree
} metaData;


RC initIndexManager(void *mgmtData) {
    printf("Index Manager was born\n");
    return RC_OK;
}

// Shutdown the B+ Tree index manager
RC shutdownIndexManager(void *mgmtData) {
    printf("Index Manager was killed\n");
    return RC_OK;
}

void * createNode(int n, bool is_leaf, bool is_root) {
    node * newNode = (node *) malloc(sizeof(node));
    newNode->keys = (int *) malloc(sizeof(int)); // Fixed DT_INT
    newNode->ptrs = (void **) malloc(sizeof(void *));
    newNode->num_keys = 0;
    newNode->is_leaf = is_leaf;
    newNode->next_leaf = NULL;
    newNode->parent = NULL;
    newNode->is_root = is_root;
    newNode->max_keys_per_node = n;
    newNode->max_ptrs_per_node = n + 1;
    return newNode;
}

RC createBtree(char *idxId, DataType keyType, int n) {
    // Initialize a new pagefile for holding the index
    createPageFile(idxId); // creates pagefile, writes /0' bytes, closes pagefile

    metaData *meta_data = (metaData *) malloc(sizeof(metaData));
    meta_data->order = n; // Setting the order
    meta_data->type = keyType; // Setting the keytype for the key
    meta_data->entries = 0; // Sum of all entries inside all the nodes

    node * root = createNode(meta_data->order,true, true); // This node is a root, which is by default a leaf as well.
    meta_data->root = root;
    meta_data->nodes = 1; // ROOT node

    // Need to write this data to the 1st (0th index) page of the file.
    // createPageFile already initialized the page by setting '/0' bytes
    SM_FileHandle file_handle;
    openPageFile(idxId, &file_handle);

    // How do we store it?
    // Serialize it, then deserialize it, making use of the struct
    // OR
    // Make use of offsets, fixed like PAGE_SIZE to logically partition page

    //Serialize -> Deserialize approach

    writeBlock(0,&file_handle,meta_data);

    // no need to check if the struct size is larger than PAGE_SIZE here,
    // as the test cases are guaranteed to be small

    // Data has been written to the first page now
    // Clean up

    free(meta_data);
    closePageFile(&file_handle);

    return RC_OK;

}

RC openBtree(BTreeHandle **tree, char *idxId) {

    SM_FileHandle file_handle;
    openPageFile(idxId, &file_handle);
    metaData *meta_data = (metaData *) malloc(sizeof(metaData));

    readBlock(0, &file_handle, meta_data);

    BTreeHandle *btree = (BTreeHandle *) malloc(sizeof(BTreeHandle));
    btree->idxId = idxId; // Storing filename in BTreeHandle
    btree->mgmtData = &meta_data;

    // printf("%d\n",btree->mgmtData);

    *tree = btree;

    closePageFile(&file_handle);
    return RC_OK;
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
    // Find correct leaf node L for k
    // We'll have to load root first, as it holds pointers to all other nodes
    // Root is stored in our metadata

    BTreeHandle *btree = (BTreeHandle *) malloc(sizeof(BTreeHandle));
    printf("File name from *tree -> %s\n",tree->idxId);
    // Add new entry into L in sorted order
        // If L has enough space, DONE
        // Otherwise split L into two nodes L and L1
        // Redistribute entries evenly and copy up middle key
        // Insert index entry pointing to L1 into parent of L
    /*
    * inner node: If, during the redistribution and copying of the middle key,
        the parent node of L becomes full, a similar split operation occurs for the
        parent node.
        Entries are redistributed evenly between the parent node and a new node
        created as a result of the split.
        The middle key from this split is pushed up further to the parent’s
        parent node.
        This process continues recursively up the tree until it reaches the root
        node.
     */
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