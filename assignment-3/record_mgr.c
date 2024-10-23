#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "tables.h"
#include "expr.h"
#include "dberror.h"

// Struct for storing table and scan information
typedef struct RM_TableMgmtData {
    BM_BufferPool bufferPool;
    BM_PageHandle pageHandle;
    int numTuples;
    int firstFreePage;
} RM_TableMgmtData;

typedef struct RM_ScanMgmtData {
    int currentPage;
    int currentSlot;
    Expr *cond;
} RM_ScanMgmtData;

// Initialize and Shutdown the Record Manager
RC initRecordManager(void *mgmtData) {
    initStorageManager();
    return RC_OK;
}

RC shutdownRecordManager() {
    return RC_OK;
}

// Table management functions
RC createTable(char *name, Schema *schema) {
    SM_FileHandle fh;
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) malloc(sizeof(RM_TableMgmtData));

    // Create the page file for the table
    createPageFile(name);
    openPageFile(name, &fh);

    // Initialize buffer manager
    initBufferPool(&tableMgmtData->bufferPool, name, 3, RS_FIFO, NULL);

    // Write schema and table info to first page
    BM_PageHandle page;
    pinPage(&tableMgmtData->bufferPool, &page, 0);
    memcpy(page.data, schema, sizeof(Schema));
    markDirty(&tableMgmtData->bufferPool, &page);
    unpinPage(&tableMgmtData->bufferPool, &page);

    // Close file
    closePageFile(&fh);
    return RC_OK;
}

RC openTable(RM_TableData *rel, char *name) {
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) malloc(sizeof(RM_TableMgmtData));

    // Initialize buffer manager
    initBufferPool(&tableMgmtData->bufferPool, name, 3, RS_FIFO, NULL);

    // Read schema from first page
    BM_PageHandle page;
    pinPage(&tableMgmtData->bufferPool, &page, 0);
    Schema *schema = (Schema *) malloc(sizeof(Schema));
    memcpy(schema, page.data, sizeof(Schema));
    unpinPage(&tableMgmtData->bufferPool, &page);

    // Set up table data
    rel->schema = schema;
    rel->mgmtData = tableMgmtData;
    rel->name = name;

    return RC_OK;
}

RC closeTable(RM_TableData *rel) {
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) rel->mgmtData;

    // Shutdown buffer pool and free memory
    shutdownBufferPool(&tableMgmtData->bufferPool);
    free(tableMgmtData);
    return RC_OK;
}

RC deleteTable(char *name) {
    // Destroy the page file
    destroyPageFile(name);
    return RC_OK;
}

int getNumTuples(RM_TableData *rel) {
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) rel->mgmtData;
    return tableMgmtData->numTuples;
}

// Record management functions
RC insertRecord(RM_TableData *rel, Record *record) {
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) rel->mgmtData;
    BM_PageHandle page;

    // Find the first free page or allocate a new one
    int pageNum = tableMgmtData->firstFreePage;
    pinPage(&tableMgmtData->bufferPool, &page, pageNum);

    // Insert record into the page (need to manage free slots)
    int slot = 0; // Find the next free slot
    record->id.page = pageNum;
    record->id.slot = slot;

    // Write record to the page
    memcpy(page.data + slot * getRecordSize(rel->schema), record->data, getRecordSize(rel->schema));

    // Update page and unpin
    markDirty(&tableMgmtData->bufferPool, &page);
    unpinPage(&tableMgmtData->bufferPool, &page);

    // Update table management data
    tableMgmtData->numTuples++;

    return RC_OK;
}

RC deleteRecord(RM_TableData *rel, RID id) {
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) rel->mgmtData;
    BM_PageHandle page;

    // Find the page by id and delete the record (mark slot as free)
    pinPage(&tableMgmtData->bufferPool, &page, id.page);

    // Mark the slot as free (implementation-specific: set slot to null or 0)
    memset(page.data + id.slot * getRecordSize(rel->schema), 0, getRecordSize(rel->schema));

    // Update page and unpin
    markDirty(&tableMgmtData->bufferPool, &page);
    unpinPage(&tableMgmtData->bufferPool, &page);

    return RC_OK;
}

RC updateRecord(RM_TableData *rel, Record *record) {
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) rel->mgmtData;
    BM_PageHandle page;

    // Find the page by RID and update the record
    pinPage(&tableMgmtData->bufferPool, &page, record->id.page);
    memcpy(page.data + record->id.slot * getRecordSize(rel->schema), record->data, getRecordSize(rel->schema));

    // Mark page as dirty and unpin
    markDirty(&tableMgmtData->bufferPool, &page);
    unpinPage(&tableMgmtData->bufferPool, &page);

    return RC_OK;
}

RC getRecord(RM_TableData *rel, RID id, Record *record) {
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) rel->mgmtData;
    BM_PageHandle page;

    // Find the page by RID and retrieve the record
    pinPage(&tableMgmtData->bufferPool, &page, id.page);
    memcpy(record->data, page.data + id.slot * getRecordSize(rel->schema), getRecordSize(rel->schema));

    // Unpin page
    unpinPage(&tableMgmtData->bufferPool, &page);

    return RC_OK;
}

// Scan management functions
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    scan->rel = rel;
    scan->mgmtData = malloc(sizeof(RM_ScanMgmtData));

    // Initialize scan management data
    RM_ScanMgmtData *scanMgmtData = (RM_ScanMgmtData *) scan->mgmtData;
    scanMgmtData->currentPage = 0;
    scanMgmtData->currentSlot = 0;
    scanMgmtData->cond = cond;

    return RC_OK;
}

RC next(RM_ScanHandle *scan, Record *record) {
    RM_ScanMgmtData *scanMgmtData = (RM_ScanMgmtData *) scan->mgmtData;
    RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData *) scan->rel->mgmtData;
    BM_PageHandle page;

    // Scan for the next record
    for (int pageNum = scanMgmtData->currentPage; pageNum < tableMgmtData->numTuples; pageNum++) {
        pinPage(&tableMgmtData->bufferPool, &page, pageNum);
        // Check each slot in the page
        for (int slot = scanMgmtData->currentSlot; slot < getRecordSize(scan->rel->schema); slot++) {
            memcpy(record->data, page.data + slot * getRecordSize(scan->rel->schema), getRecordSize(scan->rel->schema));
            unpinPage(&tableMgmtData->bufferPool, &page);

            // If the record matches the scan condition, return it
            Value *result;
            evalExpr(record, scan->rel->schema, scanMgmtData->cond, &result);
            if (result->v.boolV) {
                scanMgmtData->currentPage = pageNum;
                scanMgmtData->currentSlot = slot + 1;
                return RC_OK;
            }
        }
    }
    return RC_RM_NO_MORE_TUPLES;
}

RC closeScan(RM_ScanHandle *scan) {
    // Free memory used by scan
    free(scan->mgmtData);
    return RC_OK;
}

// Helper functions for schemas
int getRecordSize(Schema *schema) {
    int size = 0;
    for (int i = 0; i < schema->numAttr; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT:
                size += sizeof(int);
                break;
            case DT_FLOAT:
                size += sizeof(float);
                break;
            case DT_STRING:
                size += schema->typeLength[i];
                break;
            case DT_BOOL:
                size += sizeof(bool);
                break;
        }
    }
    return size;
}

Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
    Schema *schema = (Schema *) malloc(sizeof(Schema));
    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;
    return schema;
}

RC freeSchema(Schema *schema) {
    free(schema);
    return RC_OK;
}

// Attribute management functions
RC createRecord(Record **record, Schema *schema) {
    *record = (Record *) malloc(sizeof(Record));
    (*record)->data = (char *) malloc(getRecordSize(schema));
    return RC_OK;
}

RC freeRecord(Record *record) {
    free(record->data);
    free(record);
    return RC_OK;
}

RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) {
    int offset = 0;
    for (int i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT:
                offset += sizeof(int);
                break;
            case DT_FLOAT:
                offset += sizeof(float);
                break;
            case DT_STRING:
                offset += schema->typeLength[i];
                break;
            case DT_BOOL:
                offset += sizeof(bool);
                break;
        }
    }

    *value = (Value *) malloc(sizeof(Value));
    (*value)->dt = schema->dataTypes[attrNum];

    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            memcpy(&((*value)->v.intV), record->data + offset, sizeof(int));
            break;
        case DT_FLOAT:
            memcpy(&((*value)->v.floatV), record->data + offset, sizeof(float));
            break;
        case DT_STRING:
            (*value)->v.stringV = (char *) malloc(schema->typeLength[attrNum]);
            memcpy((*value)->v.stringV, record->data + offset, schema->typeLength[attrNum]);
            break;
        case DT_BOOL:
            memcpy(&((*value)->v.boolV), record->data + offset, sizeof(bool));
            break;
    }

    return RC_OK;
}

RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
    int offset = 0;
    for (int i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT:
                offset += sizeof(int);
                break;
            case DT_FLOAT:
                offset += sizeof(float);
                break;
            case DT_STRING:
                offset += schema->typeLength[i];
                break;
            case DT_BOOL:
                offset += sizeof(bool);
                break;
        }
    }

    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            memcpy(record->data + offset, &(value->v.intV), sizeof(int));
            break;
        case DT_FLOAT:
            memcpy(record->data + offset, &(value->v.floatV), sizeof(float));
            break;
        case DT_STRING:
            memcpy(record->data + offset, value->v.stringV, schema->typeLength[attrNum]);
            break;
        case DT_BOOL:
            memcpy(record->data + offset, &(value->v.boolV), sizeof(bool));
            break;
    }

    return RC_OK;
}
