#include "btree_mgr.h"

#include <ctype.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// // Helper function to recursively print the tree structure
// void printNode(node *currentNode, char **output, int level) {
//     // Create indentation string based on the level of the node in the tree
//     char indent[3 * level + 1];  // At most 3 spaces per level
//     for (int i = 0; i < level; i++) {
//         indent[i * 2] = ' ';  // Add space for indentation
//         indent[i * 2 + 1] = ' ';
//     }
//     indent[3 * level] = '\0'; // Null terminate the string
//
//     // Reallocate memory and add indentation
//     size_t newSize = strlen(*output) + strlen(indent) + 1; // +1 for the null terminator
//     char *temp = realloc(*output, newSize);
//     if (temp == NULL) {
//         free(*output);  // Free previously allocated memory in case of failure
//         *output = NULL;
//         return;  // Return as reallocation failed
//     }
//     *output = temp;
//     strcat(*output, indent);  // Add the indentation to the output string
//
//     // Print whether this node is a root or a leaf
//     if (currentNode->is_root) {
//         strcat(*output, "[Root] ");
//     } else if (currentNode->is_leaf) {
//         strcat(*output, "[Leaf] ");
//     } else {
//         strcat(*output, "[Internal] ");
//     }
//
//     // Print the keys in the current node
//     newSize = strlen(*output) + 50 + currentNode->num_keys * 20; // Reserve space for keys
//     temp = realloc(*output, newSize);
//     if (temp == NULL) {
//         free(*output);  // Free previously allocated memory in case of failure
//         *output = NULL;
//         return;  // Return as reallocation failed
//     }
//     *output = temp;
//     strcat(*output, "(");  // Opening parenthesis for keys
//
//     for (int i = 0; i < currentNode->num_keys; i++) {
//         char keyStr[20];
//         sprintf(keyStr, "%d", currentNode->keys[i].v.intV);
//         strcat(*output, keyStr);
//         if (i < currentNode->num_keys - 1) {
//             strcat(*output, ", ");
//         }
//     }
//     strcat(*output, ")");  // Closing parenthesis for keys
//     strcat(*output, "\n");  // Newline after keys
//
//     // If it's a non-leaf node, recursively print the child nodes
//     if (!currentNode->is_leaf) {
//         for (int i = 0; i <= currentNode->num_keys; i++) {
//             printNode((node *)currentNode->ptrs[i], output, level + 1);
//         }
//     }
// }
//
//
// // Print the entire B+ Tree starting from the root
// char *printTree(BTreeHandle *tree) {
//     if (tree == NULL || tree->mgmtData == NULL) {
//         return strdup("Error: Tree is not initialized.");
//     }
//
//     // Access the metadata and root node
//     metaData *meta_data = (metaData *)tree->mgmtData;
//
//     // Allocate memory for the output string
//     char *output = (char *)malloc(1);
//     output[0] = '\0'; // Start with an empty string
//
//     // Print the root node and all its children
//     if (meta_data->root != NULL) {
//         printNode(meta_data->root, &output, 0);
//     } else {
//         return strdup("Tree is empty.");
//     }
//
//     return output; // Return the dynamically allocated string containing the tree structure
// }

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
    // printf("Inserting Key: %d\n", key->v.intV);
    // printf("---------- Tree before insertion ----------\n");
    // char *treeStructure_before = printTree(tree);
    // printf("%s", treeStructure_before);
    // free(treeStructure_before); // Free the allocated memory after use

    metaData *meta_data = (metaData *)tree->mgmtData;
    node *current_node = meta_data->root;

    // 1. Traverse the tree to find the appropriate leaf node where the key should be inserted
    while (!current_node->is_leaf) {
        // Traverse internal nodes, find the correct child pointer to follow
        int i = 0;
        while (i < current_node->num_keys && compareKeys(&key, &current_node->keys[i]) >= 0) { // compareKeys(k1,k2) - k1>k2 -> return 1 || k1 < k2 -> return -1
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
        sortKeys(current_node->keys, current_node->ptrs, numKeys);
        current_node->num_keys++;
        meta_data->entries++;


        // printf("---------- Tree after insertion ----------\n");
        // char *treeStructure_after = printTree(tree);
        // printf("%s", treeStructure_after);
        // free(treeStructure_after); // Free the allocated memory after use
        // printf("-----------------------------------------------------\n");

        return RC_OK;
    }

    // -------------------------------------------------------------------------------

    // 3. If node is overflowed, creating new node (Splitting)
    node *new_node = createNode(meta_data->order,true, false);
    new_node->parent = current_node->parent;

    // ----------------------------------------------------------------
    // Add the new key in existing keys array, then sort, then split
    // meta_data->Entries++ karna hai
    // ----------------------------------------------------------------

    int mid = current_node->max_keys_per_node / 2;
    for (int i = mid + 1; i < current_node->num_keys; i++) {
        new_node->keys[i - (mid + 1)] = current_node->keys[i];
        new_node->ptrs[i - (mid + 1)] = current_node->ptrs[i];
        new_node->num_keys++;
        meta_data->entries++;
    }

    if (compareKeys(&key, &current_node->keys[mid]) >= 0) {
        // Insert new key after copying
        int insert_pos = new_node->num_keys;
        new_node->keys[insert_pos] = *key;
        new_node->ptrs[insert_pos] = (void *)(&rid);
        new_node->num_keys++;
        meta_data->entries++;
        sortKeys(new_node->keys, new_node->ptrs, new_node->num_keys);
    }

    current_node->num_keys = mid + 1;

    if (current_node->is_root == true) {
        node *new_root = createNode(meta_data->order,false, true);

        new_root->keys[0] = new_node->keys[0];
        new_root->num_keys++;
        meta_data->entries++;
        new_root->ptrs[0] = current_node;
        new_root->ptrs[1] = new_node;

        current_node->parent = new_root;
        new_node->parent = new_root;
        current_node->is_root = false;

        meta_data->root = new_root;
    }
    // ----------------------------------------------------------------
    // Infinite loop ho raha hai because we are not handling non-leaf node. If current_node is roo
    // ----------------------------------------------------------------
    else {
        node *parent_node = (node *) current_node->parent;
        printf("parent->num_keys: %d\n", parent_node->num_keys);
        for (int i = 0; i < current_node->parent->num_keys; i++) {
            printf("%d, ",current_node->parent->keys[i].v.intV);
        }
        printf("\n");
        int parent_insert_pos = parent_node->num_keys;
        printf("%d\n",parent_insert_pos);
        parent_node->keys[parent_insert_pos] = *key;
        parent_node->num_keys++;
        // insertKey(tree, &new_node->keys[0], rid);  // Recursive call to insert into the parent
    }

    // printf("---------- Tree after insertion ----------\n");
    // char *treeStructure_after = printTree(tree);
    // printf("%s", treeStructure_after);
    // free(treeStructure_after); // Free the allocated memory after use
    // printf("-----------------------------------------------------\n");
    return RC_OK;
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