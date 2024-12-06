#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"

#include <limits.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "tables.h"
#include "dberror.h"
#include "expr.h"

// table and manager
RC initRecordManager (void *mgmtData) {
    initStorageManager();
    return 0;
}
RC shutdownRecordManager (){return RC_OK;}


RC createTable(char *name, Schema *schema) {
    // Construct the file name for the table
    char local_fname[64] = {'\0'};
    strcat(local_fname, name);
    // strcat(local_fname, ".bin");

    // Create the page file for the table
    RC rc = createPageFile(local_fname);
    if (rc != RC_OK) {
        return rc;  // Return error if page file creation fails
    }

    // Initialize buffer pool for managing pages
    BM_BufferPool *buffer_pool = MAKE_POOL();
    rc = initBufferPool(buffer_pool, local_fname, 4, RS_FIFO, NULL);  // 4 pages, FIFO replacement
    if (rc != RC_OK) {
        return rc;  // Return error if buffer pool initialization fails
    }

    // Serialize the schema
    char *serializedSchema = serializeSchema(schema);
    // printf("Serialized schema: %s\n", serializedSchema);
    if (serializedSchema == NULL) {
        shutdownBufferPool(buffer_pool);
        return -1;  // Handle serialization failure
    }

    // Pin the first page, write serialized schema to it, and mark it as dirty
    BM_PageHandle *page = MAKE_PAGE_HANDLE();
    rc = pinPage(buffer_pool, page, 0);  // First page reserved for schema
    if (rc != RC_OK) {
        free(serializedSchema);
        shutdownBufferPool(buffer_pool);
        return rc;  // Handle pinning error
    }

    // Copy serialized schema data into the page's data field
    strncpy(page->data, serializedSchema, PAGE_SIZE);  // Assuming schema fits within one page
    markDirty(buffer_pool, page);  // Mark page as dirty to ensure it's written to disk

    // Step 6: Unpin the page and force it to disk, then free resources
    rc = unpinPage(buffer_pool, page);
    if (rc != RC_OK) {
        free(serializedSchema);
        shutdownBufferPool(buffer_pool);
        return rc;  // Handle unpinning error
    }

    rc = forcePage(buffer_pool, page);  // Ensure data is written to disk
    if (rc != RC_OK) {
        free(serializedSchema);
        shutdownBufferPool(buffer_pool);
        return rc;  // Handle force page error
    }
    free(serializedSchema);

    int numTuples = 0;
    rc = pinPage(buffer_pool, page, 1);
    if (rc != RC_OK) {
        shutdownBufferPool(buffer_pool);
        return rc;  // Handle pinning error
    }
    memcpy(page->data, &numTuples, sizeof(int));
    markDirty(buffer_pool, page);
    unpinPage(buffer_pool, page);
    forcePage(buffer_pool, page);

    // ----------------------- DEBUGGING -----------------------
    SM_FileHandle fh;
    char buffer[PAGE_SIZE];
    if (openPageFile(local_fname, &fh) == RC_OK) {
        if (readBlock(0, &fh, buffer) == RC_OK) {
            // printf("Contents of page 0 on disk: %s\n", buffer);
        } else {
            // printf("Failed to read page 0 from disk\n");
        }
        closePageFile(&fh);
    }
    // ----------------------- DEBUGGING -----------------------
    // Clean up resources
    shutdownBufferPool(buffer_pool);  // Shutdown buffer pool after write

    if (openPageFile(local_fname, &fh) == RC_OK) {
        if (readBlock(0, &fh, buffer) == RC_OK) {
            // printf("Contents of page 0 on disk (after createTable): %s\n", buffer);
        } else {
            // printf("Failed to read page 0 from disk\n");
        }
        closePageFile(&fh);
    }

    return RC_OK;  // Successfully created the table
}


Schema *deserializeSchema(char *data) {
    if (data == NULL) {
        return NULL;
    }

    Schema *schema = (Schema *)malloc(sizeof(Schema));
    if (schema == NULL) {
        return NULL;  // Memory allocation failed
    }

    // Initial parsing: locate the number of attributes
    char *pos = strstr(data, "with <");
    if (pos == NULL) return NULL;
    pos += strlen("with <");

    // Extract number of attributes
    sscanf(pos, "%d", &schema->numAttr);

    // Allocate memory for schema fields based on the number of attributes
    schema->attrNames = (char **)malloc(3 * sizeof(char *));
    schema->dataTypes = (DataType *)malloc(schema->numAttr * sizeof(DataType));
    schema->typeLength = (int *)malloc(schema->numAttr * sizeof(int));

    // Move to the attribute list and parse each attribute
    pos = strchr(pos, '(') + 1;
    for (int i = 0; i < schema->numAttr; i++) {
        // Read attribute name until colon
        char attrName[50];
        sscanf(pos, " %[^:]:", attrName);

        // ------------- DEBUGGING START ----------------

        schema->attrNames[i] = (char *)malloc(strlen(attrName) + 1);
        if (schema->attrNames[i] == NULL) {
            // Handle memory allocation failure
            fprintf(stderr, "Memory allocation failed for attrNames[%d]\n", i);
            return NULL;
        }
        strcpy(schema->attrNames[i], attrName);
        // printf("Parsed attribute name: %s\n", schema->attrNames[i]);



        // ------------- DEBUGGING END ----------------

        // Move to the type and parse it
        pos = strchr(pos, ':') + 2;  // Move past ': '
        if (strncmp(pos, "INT", 3) == 0) {
            schema->dataTypes[i] = DT_INT;
            schema->typeLength[i] = 0;
            pos += 3;  // Move past "INT"
        } else if (strncmp(pos, "FLOAT", 5) == 0) {
            schema->dataTypes[i] = DT_FLOAT;
            schema->typeLength[i] = 0;
            pos += 5;  // Move past "FLOAT"
        } else if (strncmp(pos, "BOOL", 4) == 0) {
            schema->dataTypes[i] = DT_BOOL;
            schema->typeLength[i] = 0;
            pos += 4;  // Move past "BOOL"
        } else if (strncmp(pos, "STRING", 6) == 0) {
            schema->dataTypes[i] = DT_STRING;
            pos += 6;  // Move past "STRING"

            // Extract string length within brackets
            if (*pos == '[') {
                int strLength;
                sscanf(pos, "[%d]", &strLength);
                schema->typeLength[i] = strLength;
                pos = strchr(pos, ']') + 1;  // Move past ']'
            }
        }
        // printf("Schema TypeLength %d\n", schema->typeLength[i]);
        // printf("Parsed DataType: %d\n", schema->dataTypes[i]);
        // Move to next attribute or end of list
        if (i < schema->numAttr - 1) {
            pos = strchr(pos, ',');
            if (pos != NULL) {
                pos++;  // Move to the start of the next attribute
            } else {
                fprintf(stderr, "Error: Expected ',' after attribute '%s'\n", attrName);
                return NULL;
            }
        }

    }

    // Parse the keys section
    pos = strstr(pos, "with keys: (");
    if (pos == NULL) {
        fprintf(stderr, "Error: Expected 'with keys: (' section.\n");
        return NULL;
    }
    pos += strlen("with keys: (");  // Move past "with keys: ("

    // Allocate memory for a single key
    schema->keySize = 1;
    schema->keyAttrs = (int *)malloc(schema->keySize * sizeof(int));
    if (schema->keyAttrs == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for key attributes.\n");
        return NULL;
    }

    // Parse the single key attribute name
    char keyName[50];
    sscanf(pos, " %[^)]", keyName);  // Read key name until the closing parenthesis

    // Find attribute index in attrNames
    schema->keyAttrs[0] = -1;  // Initialize with an invalid index to check if we find the key
    for (int j = 0; j < schema->numAttr; j++) {
        if (strcmp(schema->attrNames[j], keyName) == 0) {
            schema->keyAttrs[0] = j;  // Set the index of the key attribute
            break;
        }
    }

    // Check if the key was found
    if (schema->keyAttrs[0] == -1) {
        fprintf(stderr, "Error: Key attribute '%s' not found in attribute list.\n", keyName);
        return NULL;
    }

    return schema;
}




RC openTable(RM_TableData *rel, char *name) {
    // Step 1: Construct the file name for the table
    SM_FileHandle fh;
    char local_fname[64] = {'\0'};
    strcat(local_fname, name);
    // strcat(local_fname, ".bin");

    // Step 2: Initialize buffer pool
    BM_BufferPool *buffer_pool = MAKE_POOL();
    RC rc = initBufferPool(buffer_pool, local_fname, 4, RS_FIFO, NULL);  // 4 pages, FIFO replacement
    if (rc != RC_OK) {
        return rc;  // Return error if buffer pool initialization fails
    }
    forceFlushPool(buffer_pool);
    // Step 3: Pin the first page to read the schema
    BM_PageHandle *page = MAKE_PAGE_HANDLE();
    rc = pinPage(buffer_pool, page, 0);  // First page contains schema
    //printf("Contents of page->data in openTable before deserialization: %s\n", page->data);

    openPageFile(name, &fh);
    char buffer[PAGE_SIZE];
    //printf("Direct read from disk in openTable: %s\n", buffer);
    closePageFile(&fh);

    if (rc != RC_OK) {
        shutdownBufferPool(buffer_pool);
        return rc;  // Handle pinning error
    }

    // Step 4: Deserialize schema from page data
    //printf("Contents of page->data being passed to deserializeSchema: %s\n", buffer);

    Schema *schema = deserializeSchema(buffer);
    if (schema == NULL) {
        unpinPage(buffer_pool, page);
        shutdownBufferPool(buffer_pool);
        printf("NULL Schema\n");
        return -1;  // Handle deserialization failure
    }

    // Step 5: Populate the RM_TableData structure
    rel->name = strdup(name);   // Duplicate name string for persistence
    rel->schema = schema;       // Assign the deserialized schema
    rel->mgmtData = buffer_pool;  // Store buffer pool in mgmtData for future access

    // Step 6: Unpin the page and clean up
    unpinPage(buffer_pool, page);

    return RC_OK;  // Successfully opened the table
}


RC closeTable(RM_TableData *rel) {
    // Step 1: Free the schema if it exists
    if (rel->schema != NULL) {
        freeSchema(rel->schema);
        rel->schema = NULL;
    }

    // Step 2: Set other management data to NULL if needed
    // rel->mgmtData is reserved for additional metadata, set to NULL for cleanup
    rel->mgmtData = NULL;

    return RC_OK;
}
RC deleteTable(char *name) {
    // Use the storage manager function to delete the file
    RC rc = destroyPageFile(name);

    // Return the result of the delete operation
    if (rc != RC_OK) {
        return rc; // Return error if deletion fails
    }

    return RC_OK;
}
int getNumTuples(RM_TableData *rel) {
    SM_FileHandle fh;
    RC rc;

    // Open the page file to read the tuple count
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) {
        return -1; // Return -1 to indicate an error if file opening fails
    }

    // Allocate a buffer for reading the first page (metadata page)
    char *data = (char *) malloc(PAGE_SIZE);
    rc = readBlock(1, &fh, data);
    if (rc != RC_OK) {
        free(data);
        closePageFile(&fh);
        return -1; // Return -1 to indicate an error if reading fails
    }

    // Extract the tuple count from the metadata
    int numTuples;
    memcpy(&numTuples, data, sizeof(int));

    // Clean up: free the buffer and close the file
    free(data);
    closePageFile(&fh);

    return numTuples; // Return the retrieved tuple count
}



// handling records in a table
RC insertRecord(RM_TableData *rel, Record *record) {
    SM_FileHandle fh;
    RC rc;

    // Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Calculate record size and slots per page
    int recordSize = getRecordSize(rel->schema);
    int slotsPerPage = (PAGE_SIZE - sizeof(int)) / recordSize;

    // Read metadata page (page 1)
    char *metaData = (char *)malloc(PAGE_SIZE);
    if (metaData == NULL) {
        closePageFile(&fh);
        return RC_WRITE_FAILED;
    }
    memset(metaData, 0, PAGE_SIZE);
    rc = readBlock(1, &fh, metaData);
    if (rc != RC_OK) {
        free(metaData);
        closePageFile(&fh);
        return rc;
    }

    // Get current number of tuples
    int numTuples;
    memcpy(&numTuples, metaData, sizeof(int));

    // Calculate target page and slot
    int targetPage = 2 + (numTuples / slotsPerPage);
    int targetSlot = numTuples % slotsPerPage;

    // Read or create target page
    char *pageData = (char *)malloc(PAGE_SIZE);
    if (pageData == NULL) {
        free(metaData);
        closePageFile(&fh);
        return RC_WRITE_FAILED;
    }
    memset(pageData, 0, PAGE_SIZE);

    // Ensure capacity
    rc = ensureCapacity(targetPage, &fh);
    if (rc != RC_OK) {
        free(metaData);
        free(pageData);
        closePageFile(&fh);
        return rc;
    }

    // Read existing page data if it exists
    rc = readBlock(targetPage, &fh, pageData);
    if (rc != RC_OK && rc != RC_READ_NON_EXISTING_PAGE) {
        free(metaData);
        free(pageData);
        closePageFile(&fh);
        return rc;
    }

    // Write record data
    int offset = targetSlot * recordSize;
    memcpy(pageData + offset, record->data, recordSize);

    // Write page back
    rc = writeBlock(targetPage, &fh, pageData);
    if (rc != RC_OK) {
        free(metaData);
        free(pageData);
        closePageFile(&fh);
        return rc;
    }

    // Update number of tuples
    numTuples++;
    memcpy(metaData, &numTuples, sizeof(int));

    // Write updated metadata
    rc = writeBlock(1, &fh, metaData);
    if (rc != RC_OK) {
        free(metaData);
        free(pageData);
        closePageFile(&fh);
        return rc;
    }

    // Set record ID
    record->id.page = targetPage;
    record->id.slot = targetSlot;

    // Cleanup
    free(metaData);
    free(pageData);
    closePageFile(&fh);

    return RC_OK;
}

// Delete a record with the specified RID
RC deleteRecord(RM_TableData *rel, RID id) {
    SM_FileHandle fh;
    RC rc;

    // Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Calculate record size
    int recordSize = getRecordSize(rel->schema);

    // Read the page
    char *pageData = (char *)malloc(PAGE_SIZE);
    rc = readBlock(id.page, &fh, pageData);
    if (rc != RC_OK) {
        free(pageData);
        closePageFile(&fh);
        return rc;
    }

    // Mark the record as deleted
    int offset = id.slot * recordSize;
    char deletionMarker[] = "~!@#$";
    memcpy(pageData + offset, deletionMarker, 5);

    // Write the modified page back
    rc = writeBlock(id.page, &fh, pageData);
    free(pageData);
    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }

    // Close the file
    closePageFile(&fh);

    return RC_OK;
}
// Update a record with new data
RC updateRecord(RM_TableData *rel, Record *record) {
    SM_FileHandle fh;
    RC rc;

    // Step 1: Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Step 2: Read the page where the record resides
    char *pageBuffer = (char *) malloc(PAGE_SIZE);
    rc = readBlock(record->id.page, &fh, pageBuffer);
    if (rc != RC_OK) {
        free(pageBuffer);
        closePageFile(&fh);
        return rc;
    }

    // Step 3: Calculate the slot size and get the position of the record in the page
    int slotSize = getRecordSize(rel->schema);
    char *recordSlot = pageBuffer + record->id.slot * slotSize;

    // Step 4: Update the record data in the slot
    memcpy(recordSlot, record->data, slotSize);

    // Step 5: Write the modified page back to the file
    rc = writeBlock(record->id.page, &fh, pageBuffer);
    free(pageBuffer); // Free the buffer

    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }

    // Step 6: Close the file
    rc = closePageFile(&fh);
    if (rc != RC_OK) return rc;

    return RC_OK;
}

// Retrieve a record by its RID
RC getRecord(RM_TableData *rel, RID id, Record *record) {
    SM_FileHandle fh;
    RC rc;

    // Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Calculate record size
    int recordSize = getRecordSize(rel->schema);

    // Allocate buffer for page
    char *pageData = (char *)malloc(PAGE_SIZE);
    if (pageData == NULL) {
        closePageFile(&fh);
        return RC_WRITE_FAILED;
    }
    memset(pageData, 0, PAGE_SIZE);

    // Read the page
    rc = readBlock(id.page, &fh, pageData);
    if (rc != RC_OK) {
        free(pageData);
        closePageFile(&fh);
        return rc;
    }

    // Calculate offset
    int offset = id.slot * recordSize;

    // Check if the record is deleted
    char isDeleted[6];  // 5 characters for "~!@#$" plus null terminator
    memcpy(isDeleted, pageData + offset, 5);
    isDeleted[5] = '\0';  // Ensure null-termination
    if (strcmp(isDeleted, "~!@#$") == 0) {
        free(pageData);
        closePageFile(&fh);
        return RC_RM_NO_MORE_TUPLES;  // Or a custom error code for deleted records
    }

    // Allocate memory for record data if needed
    if (record->data == NULL) {
        record->data = (char *)malloc(recordSize);
        if (record->data == NULL) {
            free(pageData);
            closePageFile(&fh);
            return RC_WRITE_FAILED;
        }
    }
    memset(record->data, 0, recordSize);

    // Copy record data
    memcpy(record->data, pageData + offset, recordSize);
    record->id = id;

    // Cleanup
    free(pageData);
    closePageFile(&fh);

    return RC_OK;
}

// scans
typedef struct ScanMgmt {
    Expr *condition;
    int currentPage;
    int currentSlot;
    bool scanStarted;
} ScanMgmt;

RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    // Initialize scan management data
    ScanMgmt *mgmt = (ScanMgmt *)malloc(sizeof(ScanMgmt));
    if (mgmt == NULL) {
        return RC_WRITE_FAILED;
    }

    mgmt->condition = cond;
    mgmt->currentPage = 1;  // Start from first data page (page 0 is schema, page 1 is metadata)
    mgmt->currentSlot = -1; // Will be incremented to 0 in first next() call
    mgmt->scanStarted = false;

    scan->rel = rel;
    scan->mgmtData = mgmt;

    return RC_OK;
}

RC next(RM_ScanHandle *scan, Record *record) {
    ScanMgmt *mgmt = (ScanMgmt *)scan->mgmtData;
    if (mgmt == NULL) {
        return -199;
    }

    Schema *schema = scan->rel->schema;
    int recordSize = getRecordSize(schema);
    int slotsPerPage = (PAGE_SIZE - sizeof(int)) / recordSize;
    bool foundRecord = false;

    // Get total number of tuples
    int totalTuples = getNumTuples(scan->rel);
    if (totalTuples <= 0) {
        return RC_RM_NO_MORE_TUPLES;
    }

    while (!foundRecord) {
        // Move to next slot
        mgmt->currentSlot++;

        // If we've reached the end of the current page
        if (mgmt->currentSlot >= slotsPerPage) {
            mgmt->currentPage++;
            mgmt->currentSlot = 0;
        }

        // Calculate if we've gone through all possible record positions
        int currentPosition = ((mgmt->currentPage - 2) * slotsPerPage) + mgmt->currentSlot;
        if (currentPosition >= totalTuples) {
            return RC_RM_NO_MORE_TUPLES;
        }

        // Try to get the record at current position
        RID rid = {mgmt->currentPage, mgmt->currentSlot};
        RC rc = getRecord(scan->rel, rid, record);

        // Skip if record doesn't exist or is deleted
        if (rc != RC_OK) {
            continue;
        }

        // If no condition, return record
        if (mgmt->condition == NULL) {
            foundRecord = true;
            continue;
        }

        // Evaluate condition
        Value *result = NULL;
        rc = evalExpr(record, schema, mgmt->condition, &result);
        if (rc != RC_OK) {
            free(result);
            continue;
        }

        // Check if condition is satisfied
        if (result->v.boolV) {
            foundRecord = true;
        }

        free(result);
    }

    return RC_OK;
}

RC closeScan(RM_ScanHandle *scan) {
    ScanMgmt *mgmt = (ScanMgmt *)scan->mgmtData;
    if (mgmt != NULL) {
        // Don't free the condition as it might be used elsewhere
        free(mgmt);
        scan->mgmtData = NULL;
    }
    return RC_OK;
}


// Creating schema for the table
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

// Free the memory allocated for a schema
RC freeSchema(Schema *schema) {
    for (int i = 0; i < schema->numAttr; i++) {
        free(schema->attrNames[i]);
    }
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->typeLength);
    free(schema->keyAttrs);
    free(schema);
    return RC_OK;
}
RC createRecord(Record **record, Schema *schema) {
    *record = (Record *) malloc(sizeof(Record));
    (*record)->data = (char *) malloc(getRecordSize(schema));
    return RC_OK;
}

// Free memory allocated for a record
RC freeRecord(Record *record) {
    free(record->data);
    free(record);
    return RC_OK;
}

// Get the size of a record for a given schema
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
            case DT_BOOL:
                size += sizeof(bool);
            break;
            case DT_STRING:
                size += schema->typeLength[i]; // Ensure typeLength is set for strings
            break;
            default:
                // Handle unknown data type if necessary
                    break;
        }
    }
    return size;
}


// Helper function to calculate the offset of an attribute in the record data
int attrOffset(Schema *schema, int attrNum) {
    int offset = 0;
    for (int i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT:
                offset += sizeof(int);
                break;
            case DT_STRING:
                offset += schema->typeLength[i];
                break;
            case DT_FLOAT:
                offset += sizeof(float);
                break;
            case DT_BOOL:
                offset += sizeof(bool);
                break;
        }
    }
    return offset;
}

RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) {
    *value = (Value *) malloc(sizeof(Value));
    if (*value == NULL) return RC_WRITE_FAILED;

    int offset = attrOffset(schema, attrNum);
    char *attrData = record->data + offset;

    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            (*value)->dt = DT_INT;
            memcpy(&((*value)->v.intV), attrData, sizeof(int));
            break;
        case DT_STRING:
            (*value)->dt = DT_STRING;
            (*value)->v.stringV = (char *) malloc(schema->typeLength[attrNum] + 1);
            if ((*value)->v.stringV == NULL) {
                free(*value);
                return RC_WRITE_FAILED;
            }
            strncpy((*value)->v.stringV, attrData, schema->typeLength[attrNum]);
            (*value)->v.stringV[schema->typeLength[attrNum]] = '\0';
            break;
        case DT_FLOAT:
            (*value)->dt = DT_FLOAT;
            memcpy(&((*value)->v.floatV), attrData, sizeof(float));
            break;
        case DT_BOOL:
            (*value)->dt = DT_BOOL;
            memcpy(&((*value)->v.boolV), attrData, sizeof(bool));
            break;
    }
    return RC_OK;
}


RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
    if (!record || !record->data || !schema || !value || attrNum < 0 || attrNum >= schema->numAttr)
        return RC_WRITE_FAILED;

    int offset = attrOffset(schema, attrNum);
    char *attrData = record->data + offset;

    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            if (value->dt != DT_INT) return RC_WRITE_FAILED;
        memcpy(attrData, &value->v.intV, sizeof(int));
        break;
        case DT_STRING:
            if (value->dt != DT_STRING) return RC_WRITE_FAILED;
        memset(attrData, 0, schema->typeLength[attrNum]); // Clear existing data
        strncpy(attrData, value->v.stringV, schema->typeLength[attrNum]);
        break;
        case DT_FLOAT:
            if (value->dt != DT_FLOAT) return RC_WRITE_FAILED;
        memcpy(attrData, &value->v.floatV, sizeof(float));
        break;
        case DT_BOOL:
            if (value->dt != DT_BOOL) return RC_WRITE_FAILED;
        memcpy(attrData, &value->v.boolV, sizeof(bool));
        break;
        default:
            return RC_WRITE_FAILED;
    }
    return RC_OK;
}