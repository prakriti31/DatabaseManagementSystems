#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "dberror.h"
#include "tables.h"

// structure for accessing btrees
typedef struct BTreeHandle {
  DataType keyType;
  char *idxId;
  void *mgmtData;
} BTreeHandle;

typedef struct BT_ScanHandle {
  BTreeHandle *tree;
  void *mgmtData;
} BT_ScanHandle;

typedef struct node {
  Value * keys; // Array of keys
  RID * rids;
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
  node *root; // Root of the tree
} metaData;

// typedef struct scanMetaData {
//   int key;
//   int num_keys;
//   int order;
//   node * root;
// } scanMetaData;

// init and shutdown index manager
extern RC initIndexManager (void *mgmtData);
extern RC shutdownIndexManager ();

// create, destroy, open, and close an btree index
extern RC createBtree (char *idxId, DataType keyType, int n);
extern RC openBtree (BTreeHandle **tree, char *idxId);
extern RC closeBtree (BTreeHandle *tree);
extern RC deleteBtree (char *idxId);

// access information about a b-tree
extern RC getNumNodes (BTreeHandle *tree, int *result);
extern RC getNumEntries (BTreeHandle *tree, int *result);
extern RC getKeyType (BTreeHandle *tree, DataType *result);

// index access
extern RC findKey (BTreeHandle *tree, Value *key, RID *result);
extern RC insertKey (BTreeHandle *tree, Value *key, RID rid);
extern RC deleteKey (BTreeHandle *tree, Value *key);
extern RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle);
extern RC nextEntry (BT_ScanHandle *handle, RID *result);
extern RC closeTreeScan (BT_ScanHandle *handle);

// debug and test functions
// extern char *printTree (BTreeHandle *tree);

#endif // BTREE_MGR_H
