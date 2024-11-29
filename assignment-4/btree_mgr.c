#include "btree_mgr.h"

#include <ctype.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    newNode->keys = (int *) malloc(sizeof(struct Value) * n); // Fixed DT_INT // [(int), (int), .... , n]
    newNode->ptrs = (void **) malloc(sizeof(void *) * (n + 1)); // No. of pointers will always be +1 than No. of Keys
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

    node *root = createNode(meta_data->order,true, true); // This node is a root, which is by default a leaf as well.
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

    *tree = (BTreeHandle *) malloc(sizeof(BTreeHandle));
    (*tree)->idxId = idxId; // Storing filename in BTreeHandle
    (*tree)->mgmtData = meta_data;

    // Debugging code to check if data is correctly pointed by *tree
    // metaData *meta = (metaData *) (*tree)->mgmtData;
    // printf("Metadata: Order = %d, Entries = %d\n", meta->order, meta->entries);

    // printf("%d\n",btree->mgmtData);


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

int compareKeys(Value *key1, Value *key2) {
    if (key1->v.intV < key2->v.intV) return -1;
    if (key1->v.intV > key2->v.intV) return 1;
    return 0;
}

void sortKeys(Value *arr, void **ptr, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j].v.intV > arr[j + 1].v.intV) {
                // Swap the keys
                int temp_key = arr[j].v.intV;
                arr[j].v.intV = arr[j + 1].v.intV;
                arr[j + 1].v.intV = temp_key;

                // Swap the pointers
                void *temp_ptr = ptr[j];
                ptr[j] = ptr[j + 1];
                ptr[j + 1] = temp_ptr;
            }
        }
    }
}


// Insert key into the B+ Tree
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {

    metaData *meta_data = (metaData *)tree->mgmtData;
    node *current_node = meta_data->root;

    // printf("|| insertKey || Metadata: Order = %d, Entries = %d\n", meta_data->order, meta_data->entries);

    printf("current_node->num_keys = %d\n", current_node->num_keys);
    printf("current_node->is_leaf = %d\n", current_node->is_leaf);

    // 1. Traverse the tree to find the appropriate leaf node where the key should be inserted
    while (!current_node->is_leaf) {
        // Traverse internal nodes, find the correct child pointer to follow
        int i = 0;
        while (i < current_node->num_keys && compareKeys(&key, &current_node->keys[i]) >= 0) {
            i++;
        }
        current_node = (node *)current_node->ptrs[i];
    }

    // -------------------------------------------------------------------------------

    if (current_node->num_keys < current_node->max_keys_per_node) {
        printf("Before insertion: current_node->keys\n");
        for (int i = 0; i < current_node->num_keys; i++) {
            printf("%d, ", current_node->keys[i].v.intV);
        }
        printf("\n");

        // Insert the new key and the corresponding pointer (RID)
        int numKeys = current_node->num_keys;
        current_node->keys[numKeys] = *key;
        current_node->ptrs[numKeys] = (void *)(&rid);
        sortKeys(current_node->keys, current_node->ptrs, numKeys);
        current_node->num_keys++;
        meta_data->entries++;

        printf("After insertion: current_node->keys\n");
        for (int i = 0; i < current_node->num_keys; i++) {
            printf("%d, ", current_node->keys[i].v.intV);
        }
        printf("\n");
        return RC_OK;
    }

    // -------------------------------------------------------------------------------

    node *new_node = (node *)malloc(sizeof(node));

    // // 3. Handle node overflow: Split the node and propagate the middle key to the parent
    // node *new_node = (node *)malloc(sizeof(node));
    // new_node->keys = (int *)malloc(current_node->max_keys_per_node * sizeof(int));
    // new_node->ptrs = (void **)malloc(current_node->max_ptrs_per_node * sizeof(void *));
    // new_node->num_keys = 0;
    // new_node->is_leaf = current_node->is_leaf;
    // new_node->parent = current_node->parent;
    // new_node->is_root = false;

    // Find correct leaf node L for k
    // We'll have to load root first, as it holds pointers to all other nodes
    // Root is stored in our metadata


        // Used to check if we have space inside root
        // or need to go to another node

        // Add new entry into L in sorted order
        // If L has enough space, DONE


            // The root has space available
            // Now insert eh key in sorted order into the root
    // if(meta_data->root->num_keys == 0) {
    //     meta_data->root->keys[0] = key;
    //     meta_data->root->num_keys += 1;
    //     meta_data->entries += 1;
    // }

            // Need to find space for the key
            // If the node is empty key[0] = key

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