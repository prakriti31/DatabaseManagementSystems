#include "btree_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Node structure for the B+ Tree
typedef struct BTreeNode {
    bool isLeaf;            // True if the node is a leaf
    int numKeys;            // Number of keys in the node
    Value **keys;           // Array of keys
    void **pointers;        // Array of pointers (child nodes or RIDs)
    struct BTreeNode *parent; // Parent node
} BTreeNode;

// BTree management structure
typedef struct BTreeMgmtData {
    int order;              // Maximum number of keys in a node
    int numNodes;           // Total number of nodes in the tree
    int numEntries;         // Total number of entries in the tree
    BTreeNode *root;        // Pointer to the root node
} BTreeMgmtData;

typedef struct BTreeScanMgmtData {
    BTreeNode *currentNode; // Pointer to current leaf node being scanned
    int currentPos;         // Current position within keys array of the node
} BTreeScanMgmtData;

// Helper to create a new B+ Tree node
static BTreeNode *createNode(int order, bool isLeaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    node->isLeaf = isLeaf;
    node->numKeys = 0;
    node->keys = (Value **)malloc(order * sizeof(Value *));
    node->pointers = (void **)malloc((order + 1) * sizeof(void *));
    node->parent = NULL;
    return node;
}

// Free a node
static void freeNode(BTreeNode *node) {
    if (node) {
        for (int i = 0; i < node->numKeys; i++) {
            free(node->keys[i]);
        }
        free(node->keys);
        free(node->pointers);
        free(node);
    }
}

// Initialize the B+ Tree index manager
RC initIndexManager(void *mgmtData) {
    // Initialize any global resources if necessary
    if (mgmtData != NULL) {
        BTreeMgmtData *btreeMgmtData = (BTreeMgmtData *)mgmtData;
        btreeMgmtData->numNodes = 0;
        btreeMgmtData->numEntries = 0;
        btreeMgmtData->order = 0;
        btreeMgmtData->root = NULL;
    }
    return RC_OK;
}

// Shutdown the B+ Tree index manager
RC shutdownIndexManager(void *mgmtData) {
    // Free any allocated resources associated with the manager
    if (mgmtData != NULL) {
        BTreeMgmtData *btreeMgmtData = (BTreeMgmtData *)mgmtData;
        if (btreeMgmtData->root != NULL) {
            freeNode(btreeMgmtData->root);  // Free the root node
        }
        free(btreeMgmtData);  // Free the management data structure
    }
    return RC_OK;
}

RC createBtree(char *idxId, DataType keyType, int n) {
    SM_FileHandle fh;

    // Step 1: Create the page file if it doesn't exist
    if (createPageFile(idxId) != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }

    // Step 2: Open the page file for reading and writing
    if (openPageFile(idxId, &fh) != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }

    // Step 3: Initialize the B-tree management data (metadata)
    BTreeMgmtData *btree = (BTreeMgmtData *)malloc(sizeof(BTreeMgmtData));
    btree->order = n;
    btree->numNodes = 1;  // Initial number of nodes (just the root)
    btree->numEntries = 0;  // Initially, no entries
    btree->root = createNode(n, true);  // Create the root node (leaf node)

    // Step 4: Write the B-tree metadata to the file (typically on the first page)
    BM_PageHandle page;
    page.data = (char *)malloc(PAGE_SIZE);
    if (readBlock(0, &fh, page.data) != RC_OK) {
        free(page.data);
        closePageFile(&fh);
        return RC_FILE_NOT_FOUND;
    }

    int offset = 0;
    memcpy(&btree->order, page.data + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&btree->numNodes, page.data + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&btree->numEntries, page.data + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(page.data + offset, &btree->root, sizeof(BTreeNode *));  // Write the root node pointer (adjust if needed)

    // Write the updated page back to the file
    if (writeBlock(0, &fh, &page) != RC_OK) {
        closePageFile(&fh);
        return RC_FILE_NOT_FOUND;
    }

    // Step 5: Clean up and close the file
    closePageFile(&fh);
    return RC_OK;
}


RC openBtree(BTreeHandle **tree, char *idxId) {
    *tree = (BTreeHandle *)malloc(sizeof(BTreeHandle));
    (*tree)->idxId = strdup(idxId);
    (*tree)->keyType = DT_INT;

    // Allocate memory for BTreeMgmtData
    BTreeMgmtData *btree = (BTreeMgmtData *)malloc(sizeof(BTreeMgmtData));

    SM_FileHandle fh;
    if (openPageFile(idxId, &fh) != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }

    // Read the B-tree metadata (root, order, numNodes, etc.) from the file
    // Example: Read root node and order from the first page or header page
    // You need to implement this logic to deserialize the B-tree state from the file
    btree->numNodes = 1;  // Example: Read from file
    btree->numEntries = 0; // Example: Read from file
    btree->order = 3;      // Example: Read from file
    btree->root = NULL;    // Example: Read from file (initialize root node if necessary)

    (*tree)->mgmtData = btree;

    closePageFile(&fh);
    return RC_OK;
}


// Close a B+ Tree index
RC closeBtree(BTreeHandle *tree) {
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;
    freeNode(btree->root);
    free(btree);
    free(tree->idxId);
    free(tree);
    return RC_OK;
}

// Delete a B+ Tree index
RC deleteBtree(char *idxId) {
    return destroyPageFile(idxId);
}

// Get the number of nodes in the B+ Tree
RC getNumNodes(BTreeHandle *tree, int *result) {
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;
    printf("Current numNodes: %d\n", btree->numNodes); // Debugging output
    *result = btree->numNodes;
    return RC_OK;
}

// Get the number of entries in the B+ Tree
RC getNumEntries(BTreeHandle *tree, int *result) {
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;
    *result = btree->numEntries;
    return RC_OK;
}

// Get the key type of the B+ Tree
RC getKeyType(BTreeHandle *tree, DataType *result) {
    *result = tree->keyType;
    return RC_OK;
}

// Find a key in the B+ Tree
RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;
    BTreeNode *current = btree->root;

    // Check if the tree is empty
    if (!current) {
        return RC_IM_KEY_NOT_FOUND;
    }

    // Traverse down the tree to find the leaf node
    while (current && !current->isLeaf) {
        int i = 0;
        // Find the appropriate pointer for the key
        while (i < current->numKeys && key->v.intV > current->keys[i]->v.intV) {
            i++;
        }

        if (i < current->numKeys + 1) { // Validate pointer range
            current = (BTreeNode *)current->pointers[i];
        } else {
            return RC_IM_KEY_NOT_FOUND; // Invalid pointer access
        }
    }

    // If we've reached a NULL node, the key is not found
    if (!current) {
        return RC_IM_KEY_NOT_FOUND;
    }

    // Search within the leaf node
    for (int i = 0; i < current->numKeys; i++) {
        if (current->keys[i]->v.intV == key->v.intV) {
            *result = *(RID *)current->pointers[i];
            return RC_OK;
        }
    }

    // Key was not found
    return RC_IM_KEY_NOT_FOUND;
}

// Insert key into the B+ Tree
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    printf("--------------------------\n"); // Debugging output

    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;

    // If the tree is empty, initialize the root
    // printf("Root: %d\n", btree->root);
    if (!btree->root) {
        btree->root = createNode(btree->order, true);
        btree->root->keys[0] = (Value *)malloc(sizeof(Value));
        memcpy(btree->root->keys[0], key, sizeof(Value));
        btree->root->pointers[0] = malloc(sizeof(RID));
        memcpy(btree->root->pointers[0], &rid, sizeof(RID));
        btree->root->numKeys = 1;
        btree->numNodes++;  // Increment node count for root
        btree->numEntries++;
        return RC_OK;
    }

    // Traverse down the tree to find the leaf node
    BTreeNode *current = btree->root;
    // printf("Is Leaf: %d\n", current->isLeaf);
    while (!current->isLeaf) {
        int i = 0;
        while (i < current->numKeys && key->v.intV > current->keys[i]->v.intV) {
            i++;
        }
        current = (BTreeNode *)current->pointers[i];
    }

    // Check if the key already exists in the leaf node
    int i = 0;
    while (i < current->numKeys && current->keys[i]->v.intV < key->v.intV) {
        i++;
    }

    // If the key is already in the node, return error code
    if (i < current->numKeys && current->keys[i]->v.intV == key->v.intV) {
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    // Shift keys and pointers to make room for the new key
    for (int j = current->numKeys; j > i; j--) {
        current->keys[j] = current->keys[j - 1];
        current->pointers[j + 1] = current->pointers[j];
    }

    current->keys[i] = (Value *)malloc(sizeof(Value));
    memcpy(current->keys[i], key, sizeof(Value));
    current->pointers[i] = malloc(sizeof(RID));
    memcpy(current->pointers[i], &rid, sizeof(RID));
    current->numKeys++;
    btree->numEntries++;

    // If the node is full, split it
    printf("Number of keys: %d\n", current->numKeys);
    printf("Order: %d\n", btree->order);
    if (current->numKeys == btree->order) {
        BTreeNode *newNode = createNode(btree->order, current->isLeaf);
        int midIndex = current->numKeys / 2;

        // Handle even n case: left node gets the extra key
        if (btree->order % 2 == 0) {
            midIndex--;  // Move the midIndex to favor the left node
        }

        Value *midKey = current->keys[midIndex];

        // Move the second half of the keys and pointers to the new node
        newNode->numKeys = current->numKeys - midIndex - 1;
        for (int j = 0; j < newNode->numKeys; j++) {
            newNode->keys[j] = current->keys[midIndex + 1 + j];
            newNode->pointers[j] = current->pointers[midIndex + 1 + j];
        }
        current->numKeys = midIndex;

        // If the node is the root, create a new root
        if (current->parent == NULL) {
            BTreeNode *newRoot = createNode(btree->order, false);
            newRoot->keys[0] = midKey;
            newRoot->pointers[0] = current;
            newRoot->pointers[1] = newNode;
            newRoot->numKeys = 1;
            btree->root = newRoot;
            current->parent = newRoot;
            newNode->parent = newRoot;
            btree->numNodes++;  // Increment node count for new root
        } else {
            // Insert the middle key into the parent
            int i = 0;
            while (i < current->parent->numKeys && current->parent->keys[i]->v.intV < midKey->v.intV) {
                i++;
            }

            for (int j = current->parent->numKeys; j > i; j--) {
                current->parent->keys[j] = current->parent->keys[j - 1];
                current->parent->pointers[j + 1] = current->parent->pointers[j];
            }

            current->parent->keys[i] = midKey;
            current->parent->pointers[i + 1] = newNode;
            current->parent->numKeys++;
            newNode->parent = current->parent;
            btree->numNodes++;  // Increment node count for new node

            // If the parent node is full, split it recursively
            if (current->parent->numKeys == btree->order) {
                return insertKey(tree, midKey, rid); // Recursively handle split
            }
        }
    }

    return RC_OK;
}

RC deleteKey(BTreeHandle *tree, Value *key) {
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;

    // Traverse the tree to find the leaf node
    BTreeNode *current = btree->root;
    while (!current->isLeaf) {
        int i = 0;
        while (i < current->numKeys && key->v.intV > current->keys[i]->v.intV) {
            i++;
        }
        current = (BTreeNode *)current->pointers[i];
    }

    // Search for the key in the leaf node
    int i = 0;
    while (i < current->numKeys && current->keys[i]->v.intV < key->v.intV) {
        i++;
    }

    // Key not found
    if (i == current->numKeys || current->keys[i]->v.intV != key->v.intV) {
        return RC_IM_KEY_NOT_FOUND;
    }

    // Remove the key and pointer
    free(current->keys[i]);
    for (int j = i; j < current->numKeys - 1; j++) {
        current->keys[j] = current->keys[j + 1];
        current->pointers[j] = current->pointers[j + 1];
    }
    current->numKeys--;
    btree->numEntries--;

    // If the node underflows, merge or redistribute
    if (current->numKeys < btree->order / 2) {
        BTreeNode *parent = current->parent;
        if (parent == NULL) {
            // If the root is underflowed, reduce the tree height
            if (current->numKeys == 0) {
                free(btree->root);
                btree->root = current;
                current->parent = NULL;
                btree->numNodes--;
            }
        } else {
            // Try to redistribute from left or right sibling, or merge if necessary
            int idx = -1;
            for (int j = 0; j < parent->numKeys; j++) {
                if (parent->pointers[j] == current) {
                    idx = j;
                    break;
                }
            }

            BTreeNode *leftSibling = (idx > 0) ? (BTreeNode *)parent->pointers[idx - 1] : NULL;
            BTreeNode *rightSibling = (idx < parent->numKeys) ? (BTreeNode *)parent->pointers[idx + 1] : NULL;

            if (leftSibling && leftSibling->numKeys > btree->order / 2) {
                // Redistribute from the left sibling
                current->keys[current->numKeys] = parent->keys[idx - 1];
                parent->keys[idx - 1] = leftSibling->keys[leftSibling->numKeys - 1];
                current->numKeys++;
                leftSibling->numKeys--;
            } else if (rightSibling && rightSibling->numKeys > btree->order / 2) {
                // Redistribute from the right sibling
                current->keys[current->numKeys] = parent->keys[idx];
                parent->keys[idx] = rightSibling->keys[0];
                current->numKeys++;
                for (int j = 0; j < rightSibling->numKeys - 1; j++) {
                    rightSibling->keys[j] = rightSibling->keys[j + 1];
                    rightSibling->pointers[j] = rightSibling->pointers[j + 1];
                }
                rightSibling->numKeys--;
            } else {
                // Merge with a sibling
                if (leftSibling) {
                    // Merge current with leftSibling
                    leftSibling->keys[leftSibling->numKeys] = parent->keys[idx - 1];
                    for (int j = 0; j < current->numKeys; j++) {
                        leftSibling->keys[leftSibling->numKeys + 1 + j] = current->keys[j];
                        leftSibling->pointers[leftSibling->numKeys + 1 + j] = current->pointers[j];
                    }
                    leftSibling->numKeys += current->numKeys + 1;
                    parent->numKeys--;
                    for (int j = idx - 1; j < parent->numKeys; j++) {
                        parent->keys[j] = parent->keys[j + 1];
                        parent->pointers[j + 1] = parent->pointers[j + 2];
                    }

                    freeNode(current);
                } else if (rightSibling) {
                    // Merge current with rightSibling
                    current->keys[current->numKeys] = parent->keys[idx];
                    for (int j = 0; j < rightSibling->numKeys; j++) {
                        current->keys[current->numKeys + 1 + j] = rightSibling->keys[j];
                        current->pointers[current->numKeys + 1 + j] = rightSibling->pointers[j];
                    }
                    current->numKeys += rightSibling->numKeys + 1;
                    parent->numKeys--;
                    for (int j = idx; j < parent->numKeys; j++) {
                        parent->keys[j] = parent->keys[j + 1];
                        parent->pointers[j + 1] = parent->pointers[j + 2];
                    }

                    freeNode(rightSibling);
                }
            }

            // If the parent node is underflowed, handle recursively
            if (parent->numKeys < btree->order / 2) {
                return deleteKey(tree, parent->keys[0]); // Recursively delete from the parent
            }
        }
    }

    return RC_OK;
}

// Open a scan on the B+ Tree
// Open a scan on the B+ Tree
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    if (tree == NULL || tree->mgmtData == NULL) {
        return RC_IM_TREE_NOT_INITIALIZED; // Error if the tree is not initialized
    }

    // Allocate memory for the scan handle
    *handle = (BT_ScanHandle *)malloc(sizeof(BT_ScanHandle));
    (*handle)->tree = tree;

    // Initialize scan-specific metadata
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;
    BTreeNode *current = btree->root;

    // Traverse to the leftmost leaf node
    while (current != NULL && !current->isLeaf) {
        current = (BTreeNode *)current->pointers[0];
    }

    // Allocate and initialize scan management data
    BTreeScanMgmtData *scanData = (BTreeScanMgmtData *)malloc(sizeof(BTreeScanMgmtData));
    scanData->currentNode = current; // Start at the leftmost leaf node
    scanData->currentPos = 0;        // Start at the first key in the node

    (*handle)->mgmtData = scanData;

    return RC_OK; // Successfully initialized the scan
}

// Get the next entry in the scan
RC nextEntry(BT_ScanHandle *handle, RID *result) {
    if (handle == NULL || handle->mgmtData == NULL) {
        return RC_IM_SCAN_NOT_OPEN; // Error if the scan is not properly initialized
    }

    // Retrieve scan-specific metadata
    BTreeScanMgmtData *scanData = (BTreeScanMgmtData *)handle->mgmtData;

    // If we have reached the end of the tree, return RC_IM_NO_MORE_ENTRIES
    if (scanData->currentNode == NULL || scanData->currentPos >= scanData->currentNode->numKeys) {
        return RC_IM_NO_MORE_ENTRIES;
    }

    // Get the current key's RID and copy it to the result
    *result = *(RID *)scanData->currentNode->pointers[scanData->currentPos];

    // Move to the next key in the current node
    scanData->currentPos++;

    // If we reach the end of the current node, move to the next leaf node
    if (scanData->currentPos >= scanData->currentNode->numKeys) {
        scanData->currentNode = (BTreeNode *)scanData->currentNode->pointers[scanData->currentNode->numKeys];
        scanData->currentPos = 0; // Reset position for the new node
    }

    return RC_OK;
}

// Close the scan on the B+ Tree
RC closeTreeScan(BT_ScanHandle *handle) {
    if (handle == NULL) {
        return RC_IM_SCAN_NOT_OPEN; // Return an error if the scan handle is NULL
    }

    // Free any scan-specific metadata if allocated
    if (handle->mgmtData != NULL) {
        free(handle->mgmtData);
        handle->mgmtData = NULL;
    }

    // Free the scan handle itself
    free(handle);

    return RC_OK; // Indicate successful closure of the scan
}

// Print the B+ Tree structure
char *printTree(BTreeHandle *tree) {
    return strdup("(Tree Structure Placeholder)");
}
