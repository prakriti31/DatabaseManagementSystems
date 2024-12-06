#include "btree_mgr.h"
#include <math.h>
#include <ctype.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void printTree(BTreeHandle *tree) {
    metaData *meta_data = (metaData *)tree->mgmtData;
    node *current_node = meta_data->root;

    if (current_node == NULL) {
        printf("Tree is empty\n");
        return;
    }

    // Use a queue to do a level-order traversal (breadth-first traversal)
    node **queue = (node **)malloc(sizeof(node*) * (meta_data->nodes));
    int front = 0, rear = 0;

    // Enqueue the root node
    queue[rear++] = current_node;

    // Traverse the tree level by level
    while (front < rear) {
        int current_level_size = rear - front;
        printf("Level %d: ", front);

        // Process each node in the current level
        for (int i = 0; i < current_level_size; i++) {
            node *current = queue[front++];

            // Print the keys in the current node
            printf("Node: ");
            for (int j = 0; j < current->num_keys; j++) {
                printf("%d ", current->keys[j].v.intV);
            }
            printf("| ");

            // Enqueue the child nodes (if any)
            if (!current->is_leaf) {
                for (int j = 0; j <= current->num_keys; j++) {
                    queue[rear++] = (node *)current->ptrs[j];
                }
            }
        }

        printf("\n");
    }

    free(queue);
}


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
    newNode->keys = (Value *) malloc(sizeof(struct Value) * n); // Fixed DT_INT // [(int), (int), .... , n]
    newNode->ptrs = (void **) malloc(sizeof(void *) * (n + 1)); // No. of pointers will always be +1 than No. of Keys
    newNode->rids = (RID *) malloc(sizeof(struct RID) * (n + 1));
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

RC closeBtree(BTreeHandle *tree) {
    printf("We are closing Btree!!!\n");
    free(tree);
    return RC_OK;
}

// Delete a B+ Tree index
RC deleteBtree(char *idxId) {
    printf("We are deleting Btree!!!\n");
    if(remove(idxId)!=0)        //check whether the name exists
        return RC_FILE_NOT_FOUND;
    return RC_OK;
}

// Get the number of nodes in the B+ Tree
RC getNumNodes(BTreeHandle *tree, int *result) {
    metaData *meta_data = (metaData *)tree->mgmtData;
    printf("Current numNodes: %d\n", meta_data->nodes);  // Debugging output (only in DEBUG mode)

    *result = meta_data->nodes;
    return RC_OK;
}

// Get the number of entries in the B+ Tree
RC getNumEntries(BTreeHandle *tree, int *result) {
    metaData *meta_data = (metaData *)tree->mgmtData;
    printf("Current Entries: %d\n", meta_data->entries);  // Debugging output (only in DEBUG mode)

    *result = meta_data->entries;
    return RC_OK;
}


// Get the key type of the B+ Tree
RC getKeyType(BTreeHandle *tree, DataType *result) {
    *result = tree->keyType;

    return RC_OK;
}

int compareKeys(Value *key1, Value *key2) {
    if (key1->v.intV < key2->v.intV) return -1;
    if (key1->v.intV > key2->v.intV) return 1;
    return 0;
}

// Find a key in the B+ Tree
RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    // First get the root, as it the starting point for our search
    metaData *meta_data = (metaData *)tree->mgmtData;
    node *current_node = meta_data->root;

    // 1. Traverse the tree to find the appropriate leaf node
    while (!current_node->is_leaf) {
        // Traverse internal nodes, find the correct child pointer to follow
        int i = 0;
        while (i < current_node->num_keys && compareKeys(key, &current_node->keys[i]) >= 0) { // compareKeys(k1,k2) - k1>k2 -> return 1 || k1 < k2 -> return -1
            i++;
        }
        current_node = (node *)current_node->ptrs[i];
    }
    for(int j = 0; j < current_node->num_keys; j++) {
        if(compareKeys(key, &current_node->keys[j]) == 0) {
            *result = current_node->rids[j];
            return RC_OK;
        }
    }
    return RC_IM_KEY_NOT_FOUND;
}


void sortKeys(Value *key, void **ptr, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (key[j].v.intV > key[j + 1].v.intV) {
                // Swap the keys
                Value temp_key = key[j];
                key[j] = key[j + 1];
                key[j + 1] = temp_key;

                // Swap the pointers
                void *temp_ptr = ptr[j];
                ptr[j] = ptr[j + 1];
                ptr[j + 1] = temp_ptr;
            }
        }
    }
}

void sortParent(Value *key, void **ptr, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (key[j].v.intV > key[j + 1].v.intV) {

                Value temp_key = key[j];
                key[j] = key[j + 1];
                key[j + 1] = temp_key;

                // ptr swap is a bit different for parent than for leaf nodes
                void *temp_ptr = ptr[j + 1];
                ptr[j + 1] = ptr[j + 2];
                ptr[j + 2] = temp_ptr;
            }
        }
    }
}

void sortParentWhenSpace(Value *key, void **ptr, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (key[j].v.intV > key[j + 1].v.intV) {

                Value temp_key = key[j];
                key[j] = key[j + 1];
                key[j + 1] = temp_key;

                // ptr swap is a bit different for parent than for leaf nodes
                void *temp_ptr = ptr[j + 1];
                ptr[j + 1] = ptr[j + 2];
                ptr[j + 2] = temp_ptr;
            }
        }
    }
}

void insertIntoParent(node *parent,node *self, Value *key, RID rid, metaData *meta_data) {

    // When parent has space available
    if(parent->num_keys < parent->max_keys_per_node) {
        node *new_node = parent;
        int numKeys = new_node->num_keys;
        new_node->keys[numKeys] = *key;
        new_node->ptrs[numKeys+1] = (void *)(&rid);
        new_node->rids[numKeys] = rid;
        new_node->num_keys++;
        sortParentWhenSpace(new_node->keys, new_node->ptrs, numKeys + 1);
    }
    else {
        if(parent->max_keys_per_node % 2 == 0) {
            // If order (n) is even, take the mid as your new root
            // We need to sort first, then swap the values, then create new root with mid
            Value *temp_key = (Value *) malloc(sizeof(struct Value) * parent->max_keys_per_node + 1);
            void ** temp_ptr = (void **) malloc(sizeof(void *) * (parent->max_keys_per_node + 2));
            int local_num_keys = 0;

            // ---------- DEBUGGING ------------- //
            printf("\nSelf Node before insertion 1\n");
            for(int k = 0; k < self->num_keys; k++) {
                printf("%d\n", self->keys[k].v.intV);
            }
            // ---------- DEBUGGING ------------- //

            // ---------- DEBUGGING ------------- //
            printf("\nParent Node before insertion of 1\n");
            for(int k = 0; k < parent->num_keys; k++) {
                printf("%d\n", parent->keys[k].v.intV);
            }
            // ---------- DEBUGGING ------------- //
            // temp[0], temp[1] <-- parent[0], parent[1]
            for (int i = 0; i < parent->num_keys; i++) {
                temp_key[i] = parent->keys[i];
                temp_ptr[i] = parent->ptrs[i];
                local_num_keys += 1;
            }
            temp_ptr[parent->num_keys] = parent->ptrs[parent->num_keys];

            temp_key[local_num_keys] = self->keys[0];
            temp_ptr[local_num_keys + 1] = self;
            local_num_keys += 1;

            // ---------- DEBUGGING ------------- //
            printf("\ntemp_key:\n");
            for(int k = 0; k < local_num_keys; k++) {
                printf("%d\n", temp_key[k].v.intV);
            }
            // ---------- DEBUGGING ------------- //

            // Incorrect key is being sent to the parent here. Need to fix this.

            sortParent(temp_key, temp_ptr, parent->num_keys+1);

            // ---------- DEBUGGING ------------- //
            printf("\ntemp_key after sorting:\n");
            for(int k = 0; k < local_num_keys; k++) {
                printf("%d\n", temp_key[k].v.intV);
            }
            // ---------- DEBUGGING ------------- //

            int mid = parent->num_keys / 2;
            for (int i = 0; i < mid + 1; i++) {
                parent->keys[i] = temp_key[i];
                parent->ptrs[i] = temp_ptr[i];
            }
            // self->keys[0] = temp_key[mid + 1];
            // self->ptrs[0] = temp_ptr[mid + 1];

            // [Parent] [Self]
            // [13, 17] [23]

            node *parent_sibling = createNode(parent->max_keys_per_node, false, false);
            for(int i=mid+1;i < local_num_keys; i++) {
                parent_sibling->keys[parent_sibling->num_keys] = temp_key[i + parent_sibling->num_keys];
                parent_sibling->ptrs[parent_sibling->num_keys] = temp_ptr[i + parent_sibling->num_keys];
                parent_sibling->num_keys++;
            }
            parent_sibling->ptrs[parent_sibling->num_keys] = temp_ptr[mid + 1 + parent_sibling->num_keys];
            if (parent->is_root == true) {
                node *new_root = createNode(meta_data->order,false, true);
                meta_data->nodes++;
                new_root->keys[0] = parent->keys[parent->num_keys - 1];
                new_root->num_keys++;
                new_root->ptrs[0] = parent;
                new_root->ptrs[1] = parent_sibling;

                parent->parent = new_root;
                parent->ptrs[parent->num_keys] = NULL;
                parent->num_keys -= 1;

                parent_sibling->parent = new_root;
                parent->is_root = false;

                meta_data->root = new_root;
            }

        }
        else if(parent->max_keys_per_node % 2 == 1) {
            // If order (n) is even, take mid+1 as your new root
        }
    }
}

// Insert key into the B+ Tree
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    printf("Inserting Key: %d\n", key->v.intV);

    metaData *meta_data = (metaData *)tree->mgmtData;
    node *current_node = meta_data->root;

    current_node->parent = current_node;
    // 1. Traverse the tree to find the appropriate leaf node where the key should be inserted
    while (!current_node->is_leaf) {
        // Traverse internal nodes, find the correct child pointer to follow
        int i = 0;
        while (i < current_node->num_keys && compareKeys(key, &current_node->keys[i]) >= 0) { // compareKeys(k1,k2) - k1>k2 -> return 1 || k1 < k2 -> return -1
            i++;
        }
        current_node = (node *)current_node->ptrs[i];
    }

    // -------------------------------------------------------------------------------

    // 2. Inserting
    if (current_node->num_keys < current_node->max_keys_per_node) {
        // Insert the new key and the corresponding pointer (RID)
        int numKeys = current_node->num_keys;
        current_node->keys[numKeys] = *key;
        current_node->ptrs[numKeys] = (void *)(&rid);
        current_node->rids[numKeys] = (rid);
        current_node->num_keys++;
        meta_data->entries++;

        sortKeys(current_node->keys, current_node->ptrs, current_node->num_keys);

        printTree(tree);
        return RC_OK;
    }

    // -------------------------------------------------------------------------------

    // 3. If node is overflowed, creating new node (Splitting)
    node *new_node = createNode(meta_data->order,true, false);
    meta_data->nodes++;
    new_node->parent = current_node->parent; // Fails if we have one leaf node and another root node // Fixed this by setting the parent of root as the parent itself, may cause recursion related errors??
    // current_node->next_leaf = new_node;
    // ----------------------------------------------------------------
    // Add the new key in existing keys array, then sort, then split
    // meta_data->Entries++ karna hai
    // ----------------------------------------------------------------

    int mid = current_node->max_keys_per_node / 2;
    for (int i = mid + 1; i < current_node->num_keys; i++) {
        new_node->keys[i - (mid + 1)] = current_node->keys[i];
        new_node->ptrs[i - (mid + 1)] = current_node->ptrs[i];
        new_node->rids[i - (mid + 1)] = current_node->rids[i];
        new_node->num_keys++;
        meta_data->entries++;

        sortKeys(current_node->keys, current_node->ptrs, current_node->num_keys);
    }

    if (compareKeys(key, &current_node->keys[mid]) >= 0) {
        // Insert new key after copying
        int insert_pos = new_node->num_keys;
        new_node->keys[insert_pos] = *key;
        new_node->ptrs[insert_pos] = (void *)(&rid);
        new_node->rids[insert_pos] = (rid);
        new_node->num_keys++;
        meta_data->entries++;
        sortKeys(new_node->keys, new_node->ptrs, new_node->num_keys);
    }
    else { // This is when key < current_node key
        Value *temp_key = (Value *) malloc(sizeof(struct Value) * current_node->max_keys_per_node + 1);
        void ** temp_ptr = (void **) malloc(sizeof(void *) * (current_node->max_keys_per_node + 2));
        RID *temp_rid = (RID *) malloc(sizeof(struct RID) * (current_node->max_keys_per_node + 2));
        for (int i = 0; i < current_node->num_keys; i++) {
            temp_key[i] = current_node->keys[i];
            temp_ptr[i] = current_node->ptrs[i];
            temp_rid[i] = current_node->rids[i];
        }
        temp_key[current_node->num_keys] = *key;
        temp_ptr[current_node->num_keys] = (void *)(&rid);
        temp_rid[current_node->num_keys] = (rid);

        sortKeys(temp_key, temp_ptr, current_node->num_keys+1);

        for (int i = 0; i < mid + 1; i++) {
            current_node->keys[i] = temp_key[i];
            current_node->ptrs[i] = temp_ptr[i];
            current_node->rids[i] = temp_rid[i];
        }

        // for (int i = 0; i < current_node->num_keys; i++) {
        //     printf("Current: %d\n",current_node->keys[i].v.intV);
        // }

        int insert_pos = new_node->num_keys;
        new_node->keys[insert_pos] = temp_key[mid + 1];
        new_node->ptrs[insert_pos] = (void *)(&rid);
        new_node->rids[insert_pos] = (rid);

        new_node->num_keys++;

        // for (int i = 0; i < new_node->num_keys; i++) {
        //     printf("New_node: %d\n",new_node->keys[i].v.intV);
        // }
    }

    // We are not deleting entries from current_node->keys, hence limiting num_keys
    current_node->num_keys = mid + 1;

    // Current node after managing overflow
    // ---------- DEBUGGING ------------- //
    printf("\nCurrent node after handling overflow insertion 1\n");
    for(int k = 0; k < current_node->num_keys; k++) {
        printf("%d\n", current_node->keys[k].v.intV);
    }
    // ---------- DEBUGGING ------------- //

    if (current_node->is_root == true) {
        node *new_root = createNode(meta_data->order,false, true);
        meta_data->nodes++;
        new_root->keys[0] = new_node->keys[0];
        new_root->num_keys++;
        new_root->ptrs[0] = current_node;
        new_root->ptrs[1] = new_node;

        current_node->parent = new_root;
        new_node->parent = new_root;
        current_node->is_root = false;

        meta_data->root = new_root;
    }

    else {
        insertIntoParent(new_node->parent,new_node, key, rid, meta_data);
        meta_data->root->ptrs[current_node->num_keys] = new_node;
    }

    printTree(tree);
    return RC_OK;
}

void deleteKeyValue(node *node, Value *key) {
    int key_value = key->v.intV;
    int size = node->num_keys;
    // Identify the element to be removed (1) and its index
    int indexToRemove = -1;
    for (int i = 0; i < size; i++) {
        if (node->keys[i].v.intV == key_value) {
            indexToRemove = i;
            break;
        }
    }

    // If the element was found, swap it with the last element
    if (indexToRemove != -1) {
        // Swap the keys (values)
        int temp_key = node->keys[indexToRemove].v.intV;
        node->keys[indexToRemove].v.intV = node->keys[size - 1].v.intV;
        node->keys[size - 1].v.intV = temp_key;

        // Swap the pointers
        void *temp_ptr = node->ptrs[indexToRemove];
        node->ptrs[indexToRemove] = node->ptrs[size - 1];
        node->ptrs[size - 1] = temp_ptr;

        // Swap the RIDs (if needed)
        RID temp_rid = node->rids[indexToRemove];
        node->rids[indexToRemove] = node->rids[size - 1];
        node->rids[size - 1] = temp_rid;

        // node->num_keys--;
    }
}

RC deleteKey(BTreeHandle *tree, Value *key) {
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\nDeleting Key: %d\n", key->v.intV);

    metaData *meta_data = (metaData *)tree->mgmtData;
    node *current_node = meta_data->root;

    // 1. Traverse the tree to find the appropriate leaf node where the key should be inserted
    while (!current_node->is_leaf) {
        // Traverse internal nodes, find the correct child pointer to follow
        int i = 0;
        while (i < current_node->num_keys && compareKeys(key, &current_node->keys[i]) >= 0) { // compareKeys(k1,k2) - k1>k2 -> return 1 || k1 < k2 -> return -1
            i++;
        }
        current_node = (node *)current_node->ptrs[i];
    }

    if (current_node->num_keys - 1 >= (int)(floor(current_node->max_keys_per_node+1)/2)) {
        printf("num_keys: %d\n", current_node->num_keys);
        deleteKeyValue(current_node, key);
        current_node->num_keys--;
        meta_data->entries--;
        sortKeys(current_node->keys, current_node->ptrs, current_node->num_keys);
    }
    else if (current_node->num_keys -1 < (int)(floor(current_node->max_keys_per_node+1)/2)) {
        printf("num_keys: %d\n", current_node->num_keys);
        deleteKeyValue(current_node, key);
        current_node->num_keys--;
        meta_data->entries--;
    }
    printf("num_keys: %d\n", current_node->num_keys);
    printTree(tree);

    return RC_OK;
 }

// Open a scan on the B+ Tree
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    metaData *meta_data = (metaData *)tree->mgmtData;
    node *current_node = meta_data->root;

    return RC_OK;
}

// Get the next entry in the scan
RC nextEntry(BT_ScanHandle *handle, RID *result) {
    return RC_OK;
}

// Close the scan on the B+ Tree
RC closeTreeScan(BT_ScanHandle *handle) {
    free(handle);
    return RC_OK;
}