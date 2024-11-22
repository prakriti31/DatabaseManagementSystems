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

// Helper function to recursively print the tree structure
void printNode(BTreeNode *node, char **output, int level) {
    // Add indentation based on the level of the node in the tree
    for (int i = 0; i < level; i++) {
        *output = realloc(*output, strlen(*output) + 3); // Add space for "  "
        strcat(*output, "  ");
    }

    // Print parent keys if they exist
    if (node->parent != NULL) {
        *output = realloc(*output, strlen(*output) + 100); // Reserve space for parent keys
        strcat(*output, "[Parent: ");
        for (int i = 0; i < node->parent->numKeys; i++) {
            char keyStr[20];
            sprintf(keyStr, "%d", node->parent->keys[i]->v.intV);
            strcat(*output, keyStr);
            if (i < node->parent->numKeys - 1) {
                strcat(*output, ", ");
            }
        }
        strcat(*output, "] ");
    }

    // Print whether this node is a left or right child
    if (node->parent != NULL) {
        for (int i = 0; i <= node->parent->numKeys; i++) {
            if (node->parent->pointers[i] == node) {
                if (i == 0) {
                    strcat(*output, "[Left Child] ");
                } else if (i == node->parent->numKeys) {
                    strcat(*output, "[Right Child] ");
                } else {
                    strcat(*output, "[Middle Child] ");
                }
                break;
            }
        }
    }

    // Print the keys of the current node
    *output = realloc(*output, strlen(*output) + 100); // Reserve space for the keys
    strcat(*output, "(");
    for (int i = 0; i < node->numKeys; i++) {
        char keyStr[20];
        sprintf(keyStr, "%d", node->keys[i]->v.intV);
        strcat(*output, keyStr);
        if (i < node->numKeys - 1) {
            strcat(*output, ", ");
        }
    }
    strcat(*output, ")");

    // If it's a non-leaf node, recursively print the child nodes
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            printNode((BTreeNode *)node->pointers[i], output, level + 1);
        }
    }

    strcat(*output, "\n");
}

// Print the B+ Tree structure starting from the root
char *printTree(BTreeHandle *tree) {
    if (tree == NULL || tree->mgmtData == NULL) {
        return strdup("Error: Tree is not initialized.");
    }

    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;

    // Allocate memory for the output string
    char *output = (char *)malloc(1);
    output[0] = '\0'; // Start with an empty string

    // Print the root node and all its children
    if (btree->root != NULL) {
        printNode(btree->root, &output, 0);
    } else {
        return strdup("Tree is empty.");
    }

    return output; // Return the dynamically allocated string containing the tree structure
}

// Helper to create a new B+ Tree node
static BTreeNode *createNode(int order, bool isLeaf) {
    // Allocate memory for the BTreeNode
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for BTreeNode\n");
        return NULL;
    }

    // Initialize basic fields
    node->isLeaf = isLeaf;
    node->numKeys = 0;
    node->parent = NULL;

    // Allocate memory for keys and pointers
    node->keys = (Value **)malloc(order * sizeof(Value *));
    node->pointers = (void **)malloc((order + 1) * sizeof(void *));

    // Check for allocation failures
    if (!node->keys || !node->pointers) {
        fprintf(stderr, "Memory allocation failed for keys or pointers\n");
        // Free already allocated memory
        if (node->keys) free(node->keys);
        if (node->pointers) free(node->pointers);
        free(node);
        return NULL;
    }

    return node;
}

// Free a node
// Free a node and all its associated resources
static void freeNode(BTreeNode *node) {
    if (node == NULL) return; // Safeguard against null pointer

    // Free child nodes recursively if the node is not a leaf
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            freeNode((BTreeNode *)node->pointers[i]);
        }
    }

    // Free each key (if dynamically allocated)
    for (int i = 0; i < node->numKeys; i++) {
        free(node->keys[i]);
        node->keys[i] = NULL; // Nullify to prevent accidental use
    }

    // Free the arrays for keys and pointers
    free(node->keys);
    free(node->pointers);

    // Nullify arrays to ensure no accidental reuse
    node->keys = NULL;
    node->pointers = NULL;

    // Free the node itself
    free(node);
    node = NULL; // Nullify the pointer for safety
}



// Initialize the B+ Tree index manager
RC initIndexManager(void *mgmtData) {
    // Check if mgmtData is valid
    if (mgmtData == NULL) {
        // printf("-----------------------------------");
        return RC_OK; // Return an error code if mgmtData is NULL
    }

    // Cast and initialize the B+ Tree management structure
    BTreeMgmtData *btreeMgmtData = (BTreeMgmtData *)mgmtData;
    btreeMgmtData->numNodes = 0;
    btreeMgmtData->numEntries = 0;

    // Set default order to a reasonable value (e.g., 3)
    btreeMgmtData->order = 3;

    // Initialize the root to NULL
    btreeMgmtData->root = NULL;

    // Return success
    return RC_OK;
}


// Shutdown the B+ Tree index manager
RC shutdownIndexManager(void *mgmtData) {
    // Check if mgmtData is valid
    if (mgmtData == NULL) {
        return RC_NULL_ARGUMENT; // Return error if no valid management data is provided
    }

    BTreeMgmtData *btreeMgmtData = (BTreeMgmtData *)mgmtData;

    // Free the root node (if it exists)
    freeNode(btreeMgmtData->root);

    // Clear pointers for safety
    btreeMgmtData->root = NULL;

    // Free the management data structure if it was dynamically allocated
    free(btreeMgmtData);

    // Return success
    return RC_OK;
}


RC createBtree(char *idxId, DataType keyType, int n) {
    SM_FileHandle fh;

    // Step 1: Create the page file
    if (createPageFile(idxId) != RC_OK) {
        return RC_FILE_CREATION_FAILED;
    }

    // Step 2: Open the page file
    if (openPageFile(idxId, &fh) != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }

    // Step 3: Allocate and initialize the B-tree management data
    BTreeMgmtData *btree = (BTreeMgmtData *)malloc(sizeof(BTreeMgmtData));
    if (!btree) {
        closePageFile(&fh);
        return RC_MEMORY_ALLOCATION_FAIL;
    }
    btree->order = n;
    btree->numNodes = 1;          // Start with one node (the root)
    btree->numEntries = 0;        // No entries initially
    btree->root = createNode(n, true);  // Create the root node as a leaf

    // Step 4: Serialize B-tree metadata to the first page
    BM_PageHandle page;
    page.data = (char *)malloc(PAGE_SIZE);
    if (!page.data) {
        free(btree);
        closePageFile(&fh);
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    memset(page.data, 0, PAGE_SIZE);  // Initialize page with zeros

    int offset = 0;
    memcpy(page.data + offset, &btree->order, sizeof(int));  // Order of the B+ tree
    offset += sizeof(int);
    memcpy(page.data + offset, &btree->numNodes, sizeof(int));  // Number of nodes
    offset += sizeof(int);
    memcpy(page.data + offset, &btree->numEntries, sizeof(int));  // Number of entries
    offset += sizeof(int);

    // Root node page number (set to 1 as metadata is on page 0)
    int rootPage = 1;
    memcpy(page.data + offset, &rootPage, sizeof(int));
    offset += sizeof(int);

    // Step 5: Write metadata to page 0
    if (writeBlock(0, &fh, page.data) != RC_OK) {
        free(page.data);
        free(btree->root);
        free(btree);
        closePageFile(&fh);
        return RC_WRITE_FAILED;
    }

    // Step 6: Serialize and write the root node to page 1
    memset(page.data, 0, PAGE_SIZE);
    offset = 0;

    // Root node metadata
    memcpy(page.data + offset, &btree->root->isLeaf, sizeof(bool));
    offset += sizeof(bool);
    memcpy(page.data + offset, &btree->root->numKeys, sizeof(int));
    offset += sizeof(int);

    // Keys and pointers (assumes empty root)
    // No keys or pointers to serialize initially, but structure needs to be valid
    for (int i = 0; i < n; i++) {
        memset(page.data + offset, 0, sizeof(Value));  // Keys initialized to 0
        offset += sizeof(Value);
    }
    for (int i = 0; i <= n; i++) {
        memset(page.data + offset, 0, sizeof(int));  // Pointers initialized to 0
        offset += sizeof(int);
    }

    if (writeBlock(1, &fh, page.data) != RC_OK) {
        free(page.data);
        free(btree->root);
        free(btree);
        closePageFile(&fh);
        return RC_WRITE_FAILED;
    }

    // Step 7: Clean up
    free(page.data);
    free(btree->root);
    free(btree);
    closePageFile(&fh);

    return RC_OK;
}

RC openBtree(BTreeHandle **tree, char *idxId) {
    // Allocate memory for the BTreeHandle
    *tree = (BTreeHandle *)malloc(sizeof(BTreeHandle));
    if (!(*tree)) {
        return RC_MEMORY_ALLOCATION_FAIL;
    }
    (*tree)->idxId = strdup(idxId);  // Duplicate the index ID string
    if (!(*tree)->idxId) {
        free(*tree);
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    // Allocate memory for BTreeMgmtData
    BTreeMgmtData *btree = (BTreeMgmtData *)malloc(sizeof(BTreeMgmtData));
    if (!btree) {
        free((*tree)->idxId);
        free(*tree);
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    // Open the page file
    SM_FileHandle fh;
    if (openPageFile(idxId, &fh) != RC_OK) {
        free((*tree)->idxId);
        free(*tree);
        free(btree);
        return RC_FILE_NOT_FOUND;
    }

    // Read metadata from the first page
    BM_PageHandle page;
    page.data = (char *)malloc(PAGE_SIZE);
    if (!page.data) {
        closePageFile(&fh);
        free((*tree)->idxId);
        free(*tree);
        free(btree);
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    if (readBlock(0, &fh, page.data) != RC_OK) {
        free(page.data);
        closePageFile(&fh);
        free((*tree)->idxId);
        free(*tree);
        free(btree);
        return RC_READ_FAILED;
    }

    // Deserialize metadata from the first page
    int offset = 0;
    memcpy(&btree->order, page.data + offset, sizeof(int));  // Order of the B+ tree
    offset += sizeof(int);
    memcpy(&btree->numNodes, page.data + offset, sizeof(int));  // Number of nodes
    offset += sizeof(int);
    memcpy(&btree->numEntries, page.data + offset, sizeof(int));  // Number of entries
    offset += sizeof(int);

    // Read the root node page number and initialize root
    int rootPage;
    memcpy(&rootPage, page.data + offset, sizeof(int));
    offset += sizeof(int);

    free(page.data);

    // Allocate and initialize the root node if it exists
    if (rootPage > 0) {
        page.data = (char *)malloc(PAGE_SIZE);
        if (!page.data) {
            closePageFile(&fh);
            free((*tree)->idxId);
            free(*tree);
            free(btree);
            return RC_MEMORY_ALLOCATION_FAIL;
        }

        if (readBlock(rootPage, &fh, page.data) != RC_OK) {
            free(page.data);
            closePageFile(&fh);
            free((*tree)->idxId);
            free(*tree);
            free(btree);
            return RC_READ_FAILED;
        }

        BTreeNode *root = (BTreeNode *)malloc(sizeof(BTreeNode));
        if (!root) {
            free(page.data);
            closePageFile(&fh);
            free((*tree)->idxId);
            free(*tree);
            free(btree);
            return RC_MEMORY_ALLOCATION_FAIL;
        }

        // Deserialize root node
        offset = 0;
        memcpy(&root->isLeaf, page.data + offset, sizeof(bool));
        offset += sizeof(bool);
        memcpy(&root->numKeys, page.data + offset, sizeof(int));
        offset += sizeof(int);

        root->keys = (Value **)malloc(btree->order * sizeof(Value *));
        root->pointers = (void **)malloc((btree->order + 1) * sizeof(void *));
        root->parent = NULL;

        if (!root->keys || !root->pointers) {
            free(root);
            free(page.data);
            closePageFile(&fh);
            free((*tree)->idxId);
            free(*tree);
            free(btree);
            return RC_MEMORY_ALLOCATION_FAIL;
        }

        // Deserialize keys and pointers (adjust based on your serialization)
        for (int i = 0; i < root->numKeys; i++) {
            root->keys[i] = (Value *)malloc(sizeof(Value));
            memcpy(root->keys[i], page.data + offset, sizeof(Value));
            offset += sizeof(Value);
        }
        for (int i = 0; i <= root->numKeys; i++) {
            memcpy(&root->pointers[i], page.data + offset, sizeof(void *));
            offset += sizeof(void *);
        }

        free(page.data);
        btree->root = root;
    } else {
        btree->root = NULL;  // No root node exists
    }

    // Assign the management data to the BTreeHandle
    (*tree)->mgmtData = btree;

    // Close the page file
    closePageFile(&fh);
    return RC_OK;
}

// Close a B+ Tree index
RC closeBtree(BTreeHandle *tree) {
    if (tree == NULL) {
        return RC_OK;  // Nothing to do if the tree is already NULL
    }

    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;

    // Free the root node if it exists
    if (btree->root != NULL) {
        freeNode(btree->root);
    }

    // Free management data and idxId if they are allocated
    if (btree != NULL) {
        free(btree);
    }

    if (tree->idxId != NULL) {
        free(tree->idxId);
    }

    // Finally, free the BTreeHandle itself
    free(tree);

    return RC_OK;
}

// Delete a B+ Tree index
RC deleteBtree(char *idxId) {
    BTreeHandle *tree = NULL;
    RC rc = openBtree(&tree, idxId);  // Open the tree to free resources
    if (rc != RC_OK) {
        return rc;  // Failed to open the tree
    }

    // Cleanup any resources held by the B-tree
    closeBtree(tree);  // Free memory and resources associated with the tree

    // Now that the B-tree resources are cleaned up, remove the file
    return destroyPageFile(idxId);
}


// Get the number of nodes in the B+ Tree
RC getNumNodes(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        return RC_INVALID_HANDLE;  // Return an error if tree or result is NULL
    }

    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;
    if (btree == NULL) {
        return RC_INVALID_HANDLE;  // Check if BTreeMgmtData is valid
    }

#ifdef DEBUG
    printf("Current numNodes: %d\n", btree->numNodes);  // Debugging output (only in DEBUG mode)
#endif

    *result = btree->numNodes;
    return RC_OK;
}

// Get the number of entries in the B+ Tree
RC getNumEntries(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        return RC_INVALID_HANDLE;  // Return an error if tree or result is NULL
    }

    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;
    if (btree == NULL) {
        return RC_INVALID_HANDLE;  // Check if BTreeMgmtData is valid
    }

    *result = btree->numEntries;
    return RC_OK;
}


// Get the key type of the B+ Tree
RC getKeyType(BTreeHandle *tree, DataType *result) {
    if (tree == NULL || result == NULL) {
        return RC_INVALID_HANDLE;  // Return an error if tree or result is NULL
    }

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

        if (i < current->numKeys) { // Correct range check for pointer access
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
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;

    // If the tree is empty, initialize the root
    if (!btree->root) {
        btree->root = createNode(btree->order, true);
        btree->root->keys[0] = (Value *)malloc(sizeof(Value));
        memcpy(btree->root->keys[0], key, sizeof(Value));
        btree->root->pointers[0] = malloc(sizeof(RID));
        memcpy(btree->root->pointers[0], &rid, sizeof(RID));
        btree->root->numKeys = 1;
        btree->numNodes++;  // Increment node count for root
        btree->numEntries++;

        // Print tree after root creation
        printf("After creating root node:\n");
        char *treeStr = printTree(tree);
        printf("%s", treeStr);
        free(treeStr);

        return RC_OK;
    }

    // Traverse down the tree to find the leaf node
    BTreeNode *current = btree->root;
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

    // Print tree after key insertion (before split, if any)
    printf("After inserting key %d:\n", key->v.intV);
    char *treeStr = printTree(tree);
    printf("%s", treeStr);
    free(treeStr);

    // If the node is full, split it
    if (current->numKeys == btree->order+1) {
        BTreeNode *newNode = createNode(btree->order, current->isLeaf);
        int midIndex = current->numKeys / 2;

        if (btree->order % 2 == 0) {
            midIndex--;  // Move midIndex for even order
        }

        Value *midKey = current->keys[midIndex];

        // Move second half of keys and pointers to the new node
        newNode->numKeys = current->numKeys - midIndex - 1;
        for (int j = 0; j < newNode->numKeys; j++) {
            newNode->keys[j] = current->keys[midIndex + 1 + j];
            newNode->pointers[j] = current->pointers[midIndex + 1 + j];
        }
        current->numKeys = midIndex;

        // Print tree after node split
        printf("After splitting node:\n");
        treeStr = printTree(tree);
        printf("%s", treeStr);
        free(treeStr);

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

            // Print tree after creating a new root
            printf("After creating new root:\n");
            treeStr = printTree(tree);
            printf("%s", treeStr);
            free(treeStr);

        } else {
            // Insert middle key into the parent node
            int i = 0;
            while (i < current->parent->numKeys && current->parent->keys[i]->v.intV < midKey->v.intV) {
                i++;
            }

            // Shift parent keys and pointers to make room
            for (int j = current->parent->numKeys; j > i; j--) {
                current->parent->keys[j] = current->parent->keys[j - 1];
                current->parent->pointers[j + 1] = current->parent->pointers[j];
            }

            current->parent->keys[i] = midKey;
            current->parent->pointers[i + 1] = newNode;
            current->parent->numKeys++;
            newNode->parent = current->parent;
            btree->numNodes++;  // Increment node count for new node

            // Print tree after updating parent
            printf("After inserting middle key into parent:\n");
            treeStr = printTree(tree);
            printf("%s", treeStr);
            free(treeStr);

            // If the parent node is full, split recursively
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

    // Print tree after key removal
    printf("After removing key %d:\n", key->v.intV);
    char *treeStr = printTree(tree);
    printf("%s", treeStr);
    free(treeStr);

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

                // Print tree after redistribution from left sibling
                printf("After redistributing from left sibling:\n");
                treeStr = printTree(tree);
                printf("%s", treeStr);
                free(treeStr);
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

                // Print tree after redistribution from right sibling
                printf("After redistributing from right sibling:\n");
                treeStr = printTree(tree);
                printf("%s", treeStr);
                free(treeStr);
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

                    // Print tree after merge with left sibling
                    printf("After merging with left sibling:\n");
                    treeStr = printTree(tree);
                    printf("%s", treeStr);
                    free(treeStr);
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

                    // Print tree after merge with right sibling
                    printf("After merging with right sibling:\n");
                    treeStr = printTree(tree);
                    printf("%s", treeStr);
                    free(treeStr);
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
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    if (tree == NULL || tree->mgmtData == NULL) {
        return RC_IM_TREE_NOT_INITIALIZED; // Error if the tree is not initialized
    }

    // Allocate memory for the scan handle
    *handle = (BT_ScanHandle *)malloc(sizeof(BT_ScanHandle));
    if (*handle == NULL) {
        return RC_MEMORY_ALLOCATION_FAILED; // Error if memory allocation fails
    }
    (*handle)->tree = tree;

    // Initialize scan-specific metadata
    BTreeMgmtData *btree = (BTreeMgmtData *)tree->mgmtData;

    // Check if the tree is empty (no root node)
    if (btree->root == NULL) {
        return RC_IM_TREE_EMPTY; // Error if the tree is empty
    }

    // Start traversal from the leftmost leaf node
    BTreeNode *current = btree->root;
    while (current != NULL && !current->isLeaf) {
        current = (BTreeNode *)current->pointers[0];
    }

    // Print tree after initial scan setup
    printf("After opening tree scan, initial tree state:\n");
    char *treeStr = printTree(tree);
    printf("%s", treeStr);
    free(treeStr);

    // Allocate and initialize scan management data
    BTreeScanMgmtData *scanData = (BTreeScanMgmtData *)malloc(sizeof(BTreeScanMgmtData));
    if (scanData == NULL) {
        free(*handle); // Free the allocated scan handle before returning
        return RC_MEMORY_ALLOCATION_FAILED; // Error if memory allocation fails
    }
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
    if (scanData->currentNode == NULL) {
        return RC_IM_NO_MORE_ENTRIES; // End of the tree
    }

    // Print tree state before moving to the next entry
    printf("Before fetching next entry, current tree state:\n");
    char *treeStr = printTree(handle->tree);
    printf("%s", treeStr);
    free(treeStr);

    // If we have reached the end of the current node, move to the next leaf node
    if (scanData->currentPos >= scanData->currentNode->numKeys) {
        // Move to the next leaf node
        scanData->currentNode = (BTreeNode *)scanData->currentNode->pointers[scanData->currentNode->numKeys];

        // If we've reached the end of the tree, return no more entries
        if (scanData->currentNode == NULL) {
            return RC_IM_NO_MORE_ENTRIES;
        }

        // Reset position for the new node
        scanData->currentPos = 0;
    }

    // Get the current key's RID and copy it to the result
    *result = *(RID *)scanData->currentNode->pointers[scanData->currentPos];

    // Move to the next key in the current node
    scanData->currentPos++;

    // Print tree state after fetching the next entry
    printf("After fetching next entry, updated tree state:\n");
    treeStr = printTree(handle->tree);
    printf("%s", treeStr);
    free(treeStr);

    return RC_OK;
}

// Close the scan on the B+ Tree
RC closeTreeScan(BT_ScanHandle *handle) {
    if (handle == NULL) {
        return RC_IM_SCAN_NOT_OPEN; // Return an error if the scan handle is NULL
    }

    // Print tree state before closing the scan for debugging
    printf("Before closing tree scan, tree state:\n");
    char *treeStr = printTree(handle->tree);
    printf("%s", treeStr);
    free(treeStr);

    // Free any scan-specific metadata if allocated
    free(handle->mgmtData); // Safe to call free on NULL
    handle->mgmtData = NULL;

    // Free the scan handle itself
    free(handle);

    return RC_OK; // Indicate successful closure of the scan
}
