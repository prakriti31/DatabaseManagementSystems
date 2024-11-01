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


// ---------------- DEBUGGING START ------------

// RC createTable(char *name, Schema *schema) {
//     // Step 1: Construct the file name for the table
//     char local_fname[64] = {'\0'};
//     strcat(local_fname, name);
//     strcat(local_fname, ".bin");
//
//     // Step 2: Create the page file for the table
//     RC rc = createPageFile(local_fname);
//     if (rc != RC_OK) {
//         return rc;  // Return error if page file creation fails
//     }
//
//     // Step 3: Initialize buffer pool or page manager
//     BM_BufferPool *buffer_pool = MAKE_POOL();
//     rc = initBufferPool(buffer_pool, local_fname, 4, RS_FIFO, NULL);  // 4 pages, FIFO replacement
//     if (rc != RC_OK) {
//         return rc;  // Return error if buffer pool initialization fails
//     }
//
//     // Step 4: Serialize the schema
//     char *serializedSchema = serializeSchema(schema);
//     if (serializedSchema == NULL) {
//         shutdownBufferPool(buffer_pool);
//         return -1;  // Handle serialization failure
//     }
//
//     // Step 5: Write serialized schema to the first page of the file
//     BM_PageHandle *page = MAKE_PAGE_HANDLE();
//     rc = pinPage(buffer_pool, page, 0);  // First page reserved for schema
//     if (rc != RC_OK) {
//         free(serializedSchema);
//         shutdownBufferPool(buffer_pool);
//         return rc;  // Handle pinning error
//     }
//
//     // Copy serialized schema data into the page's data field
//     strncpy(page->data, serializedSchema, PAGE_SIZE);  // Assuming schema fits within one page
//     markDirty(buffer_pool, page);  // Mark page as dirty to ensure it's written to disk
//
//     // Step 6: Unpin and cleanup
//     unpinPage(buffer_pool, page);
//     free(serializedSchema);  // Free the serialized schema after writing
//     shutdownBufferPool(buffer_pool);  // Shutdown buffer pool after write
//
//     return RC_OK;  // Successfully created the table
// }

// ------------------- DEBUGGING END -----------------

// ------------------- UPDATED CREATE-TABLE FN -------------------

RC createTable(char *name, Schema *schema) {
    // Step 1: Construct the file name for the table
    char local_fname[64] = {'\0'};
    strcat(local_fname, name);
    // strcat(local_fname, ".bin");

    // Step 2: Create the page file for the table
    RC rc = createPageFile(local_fname);
    if (rc != RC_OK) {
        return rc;  // Return error if page file creation fails
    }

    // Step 3: Initialize buffer pool for managing pages
    BM_BufferPool *buffer_pool = MAKE_POOL();
    rc = initBufferPool(buffer_pool, local_fname, 4, RS_FIFO, NULL);  // 4 pages, FIFO replacement
    if (rc != RC_OK) {
        return rc;  // Return error if buffer pool initialization fails
    }

    // Step 4: Serialize the schema
    char *serializedSchema = serializeSchema(schema);
    if (serializedSchema == NULL) {
        shutdownBufferPool(buffer_pool);
        return -1;  // Handle serialization failure
    }

    // Step 5: Pin the first page, write serialized schema to it, and mark it as dirty
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
    // Clean up resources
    shutdownBufferPool(buffer_pool);  // Shutdown buffer pool after write

    return RC_OK;  // Successfully created the table
}


// ------------------- UPDATED CREATE-TABLE FN -------------------


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
        printf("Parsed attribute name: %s\n", schema->attrNames[i]);



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
        printf("Schema TypeLength %d\n", schema->typeLength[i]);
        printf("Parsed DataType: %d\n", schema->dataTypes[i]);
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
    char local_fname[64] = {'\0'};
    strcat(local_fname, name);
    // strcat(local_fname, ".bin");

    // Step 2: Initialize buffer pool
    BM_BufferPool *buffer_pool = MAKE_POOL();
    RC rc = initBufferPool(buffer_pool, local_fname, 4, RS_FIFO, NULL);  // 4 pages, FIFO replacement
    if (rc != RC_OK) {
        return rc;  // Return error if buffer pool initialization fails
    }

    // Step 3: Pin the first page to read the schema
    BM_PageHandle *page = MAKE_PAGE_HANDLE();
    rc = pinPage(buffer_pool, page, 0);  // First page contains schema
    if (rc != RC_OK) {
        shutdownBufferPool(buffer_pool);
        return rc;  // Handle pinning error
    }

    // Step 4: Deserialize schema from page data
    Schema *schema = deserializeSchema(page->data);
    if (schema == NULL) {
        unpinPage(buffer_pool, page);
        shutdownBufferPool(buffer_pool);
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
    // --------------- DEBUGGING -------------------
    printf("Entering insertRecord, record data to be inserted: ");
    for (int i = 0; i < getRecordSize(rel->schema); i++) {
        printf("%02x ", record->data[i] & 0xff);
    }
    printf("\n");
    // --------------- DEBUGGING -------------------
    SM_FileHandle fh;
    RC rc;

    // Step 1: Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Step 2: Read and update numTuples in page 1
    char *data = (char *) malloc(PAGE_SIZE);
    rc = readBlock(1, &fh, data);
    if (rc != RC_OK) {
        free(data);
        closePageFile(&fh);
        return rc;
    }

    // Debugging: Print the current tuple count
    int numTuples;
    memcpy(&numTuples, data, sizeof(int));
    printf("numTuples before increment: %d\n", numTuples);

    // Increment tuple count and update it in page 1
    numTuples++;
    memcpy(data, &numTuples, sizeof(int));
    rc = writeBlock(1, &fh, data);
    free(data);

    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }
    printf("numTuples after increment: %d\n", numTuples);

    // Step 4: Calculate slot size and slots per page
    int slotSize = getRecordSize(rel->schema);
    if (slotSize <= 0) {
        closePageFile(&fh);
        return -1;
    }
    int slotsPerPage = PAGE_SIZE / slotSize;

    // Step 5: Find an appropriate page with free space
    int pageNum = 2;  // Start from page 2
    char *pageBuffer = (char *) malloc(PAGE_SIZE);
    bool slotFound = false;
    int slot;

    while (!slotFound) {
        if (ensureCapacity(pageNum + 1, &fh) != RC_OK) {
            free(pageBuffer);
            closePageFile(&fh);
            return RC_WRITE_FAILED;
        }

        rc = readBlock(pageNum, &fh, pageBuffer);
        if (rc != RC_OK) {
            free(pageBuffer);
            closePageFile(&fh);
            return rc;
        }

        // Find an empty slot in the page
        for (slot = 0; slot < slotsPerPage; slot++) {
            if (pageBuffer[slot * slotSize] == '\0') {  // Check if slot is empty
                slotFound = true;
                break;
            }
        }

        if (!slotFound) {
            pageNum++;
        }
    }

    // Debugging: Print page and slot where record will be written
    printf("Writing to page %d, slot %d\n", pageNum, slot);

    // Step 6: Write the record data into the found slot
    // ------------- DEBUGGING -------------------
    // printf("Record data to be written: ");
    // for (int i = 0; i < slotSize; i++) {
    //     printf("%02x ", (unsigned char)record->data[i]);
    // }
    // printf("\n");
    // ------------- DEBUGGING -------------------

    memcpy(pageBuffer + slot * slotSize, record->data, slotSize);
    rc = writeBlock(pageNum, &fh, pageBuffer);
    if(rc != RC_OK) {
        free(pageBuffer);
    }
    // ------------- DEBUGGING -------------------
    // Confirm data is written by reading it back (for debugging)
    // char *verifyBuffer = (char *) malloc(PAGE_SIZE);
    // rc = readBlock(pageNum, &fh, verifyBuffer);
    // printf("Data in page %d, slot %d after write: ", pageNum, slot);
    // for (int i = slot * slotSize; i < (slot + 1) * slotSize; i++) {
    //     printf("%02x ", (unsigned char)verifyBuffer[i]);
    // }
    // printf("\n");
    // free(verifyBuffer);
    // // ------------- DEBUGGING -------------------
    free(pageBuffer);

    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }

    // Step 7: Set the Record ID
    record->id.page = pageNum;
    record->id.slot = slot;

    // Step 8: Close the file
    rc = closePageFile(&fh);
    if (rc != RC_OK) return rc;

    return RC_OK;
}


// Delete a record with the specified RID
RC deleteRecord(RM_TableData *rel, RID id) {
    SM_FileHandle fh;
    RC rc;

    // Step 1: Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Step 2: Read the page where the record resides
    char *pageBuffer = (char *) malloc(PAGE_SIZE);
    rc = readBlock(id.page, &fh, pageBuffer);
    if (rc != RC_OK) {
        free(pageBuffer);
        closePageFile(&fh);
        return rc;
    }

    // Step 3: Calculate the slot size using the schema and get the position of the record in the page
    int slotSize = getRecordSize(rel->schema);
    char *recordSlot = pageBuffer + id.slot * slotSize;

    // Step 4: Mark the slot as free by zeroing out the slot (or you can set a specific flag if preferred)
    memset(recordSlot, 0, slotSize);

    // Step 5: Write the modified page back to the file
    rc = writeBlock(id.page, &fh, pageBuffer);
    free(pageBuffer); // Free the buffer

    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }

    // Step 6: Update the tuple count in the metadata page
    char *metaData = (char *) malloc(PAGE_SIZE);
    rc = readBlock(0, &fh, metaData);
    if (rc != RC_OK) {
        free(metaData);
        closePageFile(&fh);
        return rc;
    }

    int numTuples;
    memcpy(&numTuples, metaData, sizeof(int));
    numTuples--; // Decrease the tuple count
    memcpy(metaData, &numTuples, sizeof(int)); // Update the count in metadata

    rc = writeBlock(0, &fh, metaData);
    free(metaData); // Free metadata buffer

    // Step 7: Close the file
    rc = closePageFile(&fh);
    if (rc != RC_OK) return rc;

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

    // Step 1: Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Step 2: Read the page where the record resides
    char *pageBuffer = (char *) malloc(PAGE_SIZE);
    rc = readBlock(id.page, &fh, pageBuffer);
    if (rc != RC_OK) {
        free(pageBuffer);
        closePageFile(&fh);
        return rc;
    }

    // Step 3: Calculate the slot size and locate the record's position in the page
    int slotSize = getRecordSize(rel->schema);
    char *recordSlot = pageBuffer + id.slot * slotSize;

    // Step 4: Copy the record data from the page into the provided Record structure
    record->id = id; // Set the RID in the record structure
    memcpy(record->data, recordSlot, slotSize); // Copy data to record

    // Step 5: Clean up and close the file
    free(pageBuffer); // Free the page buffer
    rc = closePageFile(&fh);
    if (rc != RC_OK) return rc;

    return RC_OK;
}

// scans

// Start a scan with a specific condition
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    scan->rel = rel;
    scan->mgmtData = cond; // Store condition for filtering
    return RC_OK;
}

// Fetch the next record that satisfies the scan condition
RC next(RM_ScanHandle *scan, Record *record) {
    // Retrieve and filter records based on the scan condition
    return RC_RM_NO_MORE_TUPLES;
}

// Close an active scan
RC closeScan(RM_ScanHandle *scan) {
    free(scan->mgmtData); // Free any allocated memory for conditions
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
static int attrOffset(Schema *schema, int attrNum) {
    int offset = 0;
    for (int i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT:
                offset += sizeof(int);
            break;
            case DT_FLOAT:
                offset += sizeof(float);
            break;
            case DT_BOOL:
                offset += sizeof(bool);
            break;
            case DT_STRING:
                offset += schema->typeLength[i];
            break;
        }
    }
    printf("Calculated offset for attribute %d: %d\n", attrNum, offset);
    return offset;
}


// Get the attribute value from a record
RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) {
    // Calculate the offset of the attribute in the record's data
    int offset = attrOffset(schema, attrNum);
    char *attrData = record->data + offset;

    // Allocate memory for the value and set the data type
    *value = (Value *) malloc(sizeof(Value));
    (*value)->dt = schema->dataTypes[attrNum];

    // Copy the attribute data based on the attribute's data type
    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            memcpy(&(*value)->v.intV, attrData, sizeof(int));
        break;
        case DT_FLOAT:
            memcpy(&(*value)->v.floatV, attrData, sizeof(float));
        break;
        case DT_BOOL:
            memcpy(&(*value)->v.boolV, attrData, sizeof(bool));
        break;
        case DT_STRING:
            (*value)->v.stringV = (char *) malloc(schema->typeLength[attrNum] + 1);
        strncpy((*value)->v.stringV, attrData, schema->typeLength[attrNum]);
        (*value)->v.stringV[schema->typeLength[attrNum]] = '\0'; // Null-terminate string
        break;
    }

    return RC_OK;
}

// Set the attribute value in a record
// RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
//     // Calculate the offset of the attribute in the record's data
//     int offset = attrOffset(schema, attrNum);
//     char *attrData = record->data + offset;
//
//     // Copy the attribute data based on the attribute's data type
//     switch (schema->dataTypes[attrNum]) {
//         case DT_INT:
//             memcpy(attrData, &value->v.intV, sizeof(int));
//         break;
//         case DT_FLOAT:
//             memcpy(attrData, &value->v.floatV, sizeof(float));
//         break;
//         case DT_BOOL:
//             memcpy(attrData, &value->v.boolV, sizeof(bool));
//         break;
//         case DT_STRING:
//             strncpy(attrData, value->v.stringV, schema->typeLength[attrNum]);
//         break;
//     }
//
//     return RC_OK;
// }

RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
    // Calculate the offset of the attribute in the record's data
    int offset = attrOffset(schema, attrNum);
    char *attrData = record->data + offset;

    // Debug: Print calculated offset
    printf("Setting attribute %d at offset %d\n", attrNum, offset);

    // Copy the attribute data based on the attribute's data type
    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            memcpy(attrData, &value->v.intV, sizeof(int));
        printf("Value set for INT: %d\n", value->v.intV);
        break;
        case DT_FLOAT:
            memcpy(attrData, &value->v.floatV, sizeof(float));
        printf("Value set for FLOAT: %f\n", value->v.floatV);
        break;
        case DT_BOOL:
            memcpy(attrData, &value->v.boolV, sizeof(bool));
        printf("Value set for BOOL: %d\n", value->v.boolV);
        break;
        case DT_STRING:
            strncpy(attrData, value->v.stringV, schema->typeLength[attrNum]);
        // Optional: null-terminate if needed
        if (schema->typeLength[attrNum] > 0) {
            attrData[schema->typeLength[attrNum] - 1] = '\0';
        }
        printf("Value set for STRING: %.*s\n", schema->typeLength[attrNum], value->v.stringV);
        break;
    }

    // Debug: Print current state of record->data after setting this attribute
    printf("record->data after setting attribute %d: ", attrNum);
    for (int i = 0; i < getRecordSize(schema); i++) {
        printf("%02x ", record->data[i] & 0xff);  // Print in hex for clarity
    }
    printf("\n");

    return RC_OK;
}