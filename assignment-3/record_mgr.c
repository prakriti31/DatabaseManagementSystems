#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
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
    SM_FileHandle fh;
    RC rc;

    // Step 1: Create a new page file for the table
    rc = createPageFile(name);
    if (rc != RC_OK) return rc;

    // Step 2: Open the newly created file
    rc = openPageFile(name, &fh);
    if (rc != RC_OK) return rc;

    // Step 3: Serialize schema information using serializeSchema from rm_serializer.c
    char *serializedSchema = serializeSchema(schema);
    int schemaSize = strlen(serializedSchema) + 1; // include null terminator

    // Step 4: Ensure schema fits within a single page
    if (schemaSize > PAGE_SIZE) {
        free(serializedSchema);
        closePageFile(&fh);
        return RC_WRITE_FAILED;
    }

    // Step 5: Write serialized schema to the first page using a buffer
    char *pageBuffer = (char *) calloc(PAGE_SIZE, sizeof(char)); // clear the buffer
    memcpy(pageBuffer, serializedSchema, schemaSize);
    rc = writeBlock(0, &fh, pageBuffer);
    free(pageBuffer);
    free(serializedSchema);

    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }

    // Step 6: Close the file after initializing schema info
    rc = closePageFile(&fh);
    if (rc != RC_OK) return rc;

    return RC_OK;
}

// Function to deserialize schema data from a page buffer
Schema *deserializeSchema(char *data) {
    Schema *schema = (Schema *) malloc(sizeof(Schema));
    if (!schema) return NULL;

    int typeLength = 0;
    char *ptr = data;
    char attrName[50];  // temporary buffer for attribute names

    // Step 1: Parse the number of attributes
    sscanf(ptr, "Schema with <%d> attributes", &schema->numAttr);

    // Move ptr to start of attribute list
    ptr = strchr(ptr, '(') + 1;

    // Step 2: Allocate memory for attributes
    schema->attrNames = (char **) malloc(sizeof(char *) * schema->numAttr);
    schema->dataTypes = (DataType *) malloc(sizeof(DataType) * schema->numAttr);
    schema->typeLength = (int *) malloc(sizeof(int) * schema->numAttr);

    if (!schema->attrNames || !schema->dataTypes || !schema->typeLength) {
        free(schema->attrNames);
        free(schema->dataTypes);
        free(schema->typeLength);
        free(schema);
        return NULL;
    }

    // Step 3: Read attribute names and types
    for (int i = 0; i < schema->numAttr; i++) {
        int scanned = sscanf(ptr, "%49[^:]: %s", attrName, attrName + strlen(attrName) + 1);
        if (scanned != 2) {
            // Error in parsing; clean up and return NULL
            for (int j = 0; j < i; j++) free(schema->attrNames[j]);
            free(schema->attrNames);
            free(schema->dataTypes);
            free(schema->typeLength);
            free(schema);
            return NULL;
        }

        schema->attrNames[i] = strdup(attrName);
        char *typeStr = attrName + strlen(attrName) + 1;

        if (strcmp(typeStr, "INT") == 0) {
            schema->dataTypes[i] = DT_INT;
            schema->typeLength[i] = 0;
        } else if (strcmp(typeStr, "FLOAT") == 0) {
            schema->dataTypes[i] = DT_FLOAT;
            schema->typeLength[i] = 0;
        } else if (strcmp(typeStr, "BOOL") == 0) {
            schema->dataTypes[i] = DT_BOOL;
            schema->typeLength[i] = 0;
        } else if (sscanf(typeStr, "STRING[%d]", &typeLength) == 1) {
            schema->dataTypes[i] = DT_STRING;
            schema->typeLength[i] = typeLength;
        }

        // Move ptr past the parsed attribute
        ptr = strchr(ptr, ',');
        if (ptr != NULL) ptr += 2; // skip ", "
    }

    // Step 4: Parse key attributes
    char *keyStart = strstr(data, "with keys: (");
    if (keyStart) {
        keyStart += strlen("with keys: (");
        schema->keySize = 0;

        // Count the number of keys
        char *keyPtr = keyStart;
        while ((keyPtr = strchr(keyPtr, ',')) != NULL) {
            schema->keySize++;
            keyPtr++;
        }
        schema->keySize++; // Account for the last key

        schema->keyAttrs = (int *) malloc(sizeof(int) * schema->keySize);

        keyPtr = keyStart;
        for (int i = 0; i < schema->keySize; i++) {
            sscanf(keyPtr, "%49[^,)]", attrName);
            keyPtr = strchr(keyPtr, ',');
            if (keyPtr != NULL) keyPtr += 2;

            // Find the index of each key attribute
            for (int j = 0; j < schema->numAttr; j++) {
                if (strcmp(schema->attrNames[j], attrName) == 0) {
                    schema->keyAttrs[i] = j;
                    break;
                }
            }
        }
    }

    return schema;
}

RC openTable(RM_TableData *rel, char *name) {
    SM_FileHandle fh;
    RC rc;

    // Step 1: Open the page file for the table
    rc = openPageFile(name, &fh);
    if (rc != RC_OK) return rc;

    // Step 2: Read the first page containing the serialized schema
    char *data = (char *) malloc(PAGE_SIZE);
    rc = readBlock(0, &fh, data);
    if (rc != RC_OK) {
        free(data);
        closePageFile(&fh);
        return rc;
    }

    // Step 3: Deserialize schema using helper from rm_serializer.c
    Schema *schema = deserializeSchema(data);  // deserialize function in rm_serializer if available
    free(data);  // Free the buffer holding raw schema data

    // Step 4: Initialize RM_TableData
    rel->name = name;
    rel->schema = schema;
    rel->mgmtData = NULL; // This could be initialized based on specific requirements

    // Step 5: Close the file
    rc = closePageFile(&fh);
    if (rc != RC_OK) return rc;

    return RC_OK;
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
    rc = readBlock(0, &fh, data);
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

    // Step 1: Open the table file
    rc = openPageFile(rel->name, &fh);
    if (rc != RC_OK) return rc;

    // Step 2: Read the first page (metadata) to get the tuple count
    char *metaData = (char *) malloc(PAGE_SIZE);
    rc = readBlock(0, &fh, metaData);
    if (rc != RC_OK) {
        free(metaData);
        closePageFile(&fh);
        return rc;
    }

    // Step 3: Get the current tuple count and increment it for the new record
    int numTuples;
    memcpy(&numTuples, metaData, sizeof(int));
    numTuples++;
    memcpy(metaData, &numTuples, sizeof(int)); // Update metadata with new tuple count
    rc = writeBlock(0, &fh, metaData);
    free(metaData); // Free metadata buffer

    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }

    // Step 4: Calculate the size of each slot based on the schema
    int slotSize = getRecordSize(rel->schema);
    int slotsPerPage = PAGE_SIZE / slotSize;

    // Step 5: Find an appropriate page with free space
    int pageNum = 1;
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
            if (pageBuffer[slot * slotSize] == '\0') { // Check if slot is empty
                slotFound = true;
                break;
            }
        }

        if (!slotFound) {
            pageNum++; // Move to next page if no free slot is found
        }
    }

    // Step 6: Write the record data into the found slot
    memcpy(pageBuffer + slot * slotSize, record->data, slotSize);
    rc = writeBlock(pageNum, &fh, pageBuffer);
    free(pageBuffer); // Free page buffer

    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }

    // Step 7: Set the Record ID with the page number and slot number
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
        size += (schema->dataTypes[i] == DT_STRING) ? schema->typeLength[i] : sizeof(int);
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
RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
    // Calculate the offset of the attribute in the record's data
    int offset = attrOffset(schema, attrNum);
    char *attrData = record->data + offset;

    // Copy the attribute data based on the attribute's data type
    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            memcpy(attrData, &value->v.intV, sizeof(int));
        break;
        case DT_FLOAT:
            memcpy(attrData, &value->v.floatV, sizeof(float));
        break;
        case DT_BOOL:
            memcpy(attrData, &value->v.boolV, sizeof(bool));
        break;
        case DT_STRING:
            strncpy(attrData, value->v.stringV, schema->typeLength[attrNum]);
        break;
    }

    return RC_OK;
}