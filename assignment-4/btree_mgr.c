#include "btree_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct BTreeNode {
    bool isLeaf;
    int numKeys;
    Value **keys;
    void **pointers;
    struct BTreeNode *parent;
} BTreeNode;

typedef struct BTreeMgmtData {
    int order;
    int numNodes;
    int numEntries;
    BTreeNode *root;
} BTreeMgmtData;

typedef struct BTreeScanMgmtData {
    BTreeNode *currentNode;
    int currentPos;
} BTreeScanMgmtData;

// Helper function to recursively print the  structure
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
}

// Initialize the B+ Tree index manager
RC initIndexManager(void *mgmtData) {
}

// Shutdown the B+ Tree index manager
RC shutdownIndexManager(void *mgmtData) {
}

RC createBtree(char *idxId, DataType keyType, int n) {
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

void splitNode(BTreeHandle *tree, BTreeNode *current) {
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