# B+ Tree Index Manager

This repository contains the implementation of a B+ Tree index manager in C. The code provides functionality to create, manage, and manipulate a B+ Tree structure for indexing purposes. The implementation includes features such as insertion, deletion, searching, and traversal of keys in the B+ Tree.

## Features

- **Index Manager Initialization**: Functions to initialize and shut down the index manager.
- **B+ Tree Operations**:
    - Creation and deletion of a B+ Tree.
    - Opening and closing a B+ Tree.
    - Inserting and deleting keys in the tree.
    - Searching for keys and retrieving associated data (RIDs).
- **Tree Traversal**:
    - Level-order traversal for debugging and visualization.
- **Scans**:
    - Open and perform scans on the tree to retrieve entries sequentially.
- **Utility Functions**:
    - Sorting keys within nodes.
    - Handling node splits during insertion.
    - Managing underflow during deletion.

## Key Functions

### Initialization
- `RC initIndexManager(void *mgmtData)`: Initializes the index manager.
- `RC shutdownIndexManager(void *mgmtData)`: Shuts down the index manager.

### B+ Tree Management
- `RC createBtree(char *idxId, DataType keyType, int n)`: Creates a new B+ Tree with a specified order (`n`) and key type.
- `RC deleteBtree(char *idxId)`: Deletes an existing B+ Tree by removing its associated file.

### Node Operations
- `void *createNode(int n, bool is_leaf, bool is_root)`: Creates a new node in the tree.
- `void insertIntoParent(node *parent, node *self, node *current_node, Value *key, RID rid, metaData *meta_data)`: Handles parent updates during node splits.

### Key Operations
- `RC insertKey(BTreeHandle *tree, Value *key, RID rid)`: Inserts a key into the tree.
- `RC deleteKey(BTreeHandle *tree, Value *key)`: Deletes a key from the tree.
- `RC findKey(BTreeHandle *tree, Value *key, RID *result)`: Searches for a key in the tree and retrieves its associated RID.

### Scans
- `RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle)`: Opens a scan on the tree for sequential access.
- `RC nextEntry(BT_ScanHandle *handle, RID *result)`: Retrieves the next entry in the scan.
- `RC closeTreeScan(BT_ScanHandle *handle)`: Closes an active scan.

### Utility Functions
- `void sortKeys(Value *key, void **ptr, RID *rids, int size)`: Sorts keys within a node.
- `int compareKeys(Value *key1, Value *key2)`: Compares two keys for ordering.



## Contributions

### Yash Vardhan Sharma
- printTree
- createNode
- closeBtree
- getNumEntries
- findKey
- sortParentWhenSpace
- deleteKeyValue
- nextEntry

### Kamakshya Nanda
- initIndexManager
- createBtree
- deleteBtree
- getKeyType
- sortKeys
- insertIntoParent
- deleteKey
- closeTreeScan

### Prakriti
- shutdownIndexManager
- openBtree
- getNumNodes
- compareKeys
- sortParent
- insertKey
- openTreeScan


# Running the Project Using Make

This guide provides instructions on how to compile, build, and clean the project using the provided `Makefile`. The `Makefile` simplifies the build process by automating the compilation of source files and linking them into an executable.

## Prerequisites

- **GNU Make**: Ensure that `make` is installed on your system.
- **GCC Compiler**: The project uses `gcc` for compilation.


## Building the Project

1. Clone the repository:
   ```bash
      git clone https://bitbucket.org/fall_24/fall_2024_32/src/master/
      cd assignment-4
   ```
2. Build Binaries:
   ```bash
       make
   ```
3. Running Tests
   After building, you can run the test cases to verify the functionality of the Record Manager.
   ```bash
   ./assignment_4
   ```
   
4. Clean
    ```bash
   make clean
   ```