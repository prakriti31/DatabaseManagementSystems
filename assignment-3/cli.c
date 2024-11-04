#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "tables.h"
#include "expr.h"

#define MAX_TABLES 10

char *createdTables[MAX_TABLES];
int tableCount = 0;

void print_menu() {
    printf("\n=== Record Manager CLI ===\n");
    printf("1. Create Table\n");
    printf("2. Open Table\n");
    printf("3. Insert Record\n");
    printf("4. Delete Record\n");
    printf("5. Update Record\n");
    printf("6. View Records\n");
    printf("7. Close Table\n");
    printf("8. Exit\n");
    printf("Choose an option: ");
}

int is_table_created(const char *tableName) {
    for (int i = 0; i < tableCount; i++) {
        if (strcmp(createdTables[i], tableName) == 0) {
            return 1;
        }
    }
    return 0;
}

void add_table(const char *tableName) {
    if (tableCount < MAX_TABLES) {
        createdTables[tableCount] = (char *)malloc(strlen(tableName) + 1);
        strcpy(createdTables[tableCount], tableName);
        tableCount++;
    } else {
        printf("Table limit reached. Cannot add more tables.\n");
    }
}

Schema *create_demo_schema() {
    int numAttr = 3;
    char **attrNames = (char **)malloc(sizeof(char *) * numAttr);
    DataType *dataTypes = (DataType *)malloc(sizeof(DataType) * numAttr);
    int *typeLength = (int *)malloc(sizeof(int) * numAttr);
    int *keys = (int *)malloc(sizeof(int) * 1);

    attrNames[0] = "ID";
    attrNames[1] = "Name";
    attrNames[2] = "Age";

    dataTypes[0] = DT_INT;
    dataTypes[1] = DT_STRING;
    dataTypes[2] = DT_INT;

    typeLength[0] = 0;
    typeLength[1] = 20;
    typeLength[2] = 0;

    keys[0] = 0;

    return createSchema(numAttr, attrNames, dataTypes, typeLength, 1, keys);
}

void create_table(char *tableName) {
    if (is_table_created(tableName)) {
        printf("Table '%s' already exists.\n", tableName);
        return;
    }

    Schema *schema = create_demo_schema();
    RC result = createTable(tableName, schema);
    if (result == RC_OK) {
        printf("Table '%s' created successfully.\n", tableName);
        add_table(tableName);
    } else {
        printf("Failed to create table '%s'. Error code: %d\n", tableName, result);
    }
}

void open_table(RM_TableData *rel, char *tableName) {
    if (!is_table_created(tableName)) {
        printf("Table '%s' does not exist. Please create it first.\n", tableName);
        return;
    }

    RC result = openTable(rel, tableName);
    if (result == RC_OK) {
        printf("Table '%s' opened successfully.\n", tableName);
    } else {
        printf("Failed to open table '%s'. Error code: %d\n", tableName, result);
    }
}

// Global variable to keep track of the next available ID
int nextID = 1;

void insert_record(RM_TableData *rel) {
    Record *record;
    createRecord(&record, rel->schema);

    Value *idVal, *nameVal, *ageVal;
    char name[20];
    int age;

    // Automatically assign the next available ID
    MAKE_VALUE(idVal, DT_INT, nextID);

    // Prompt user for Name and Age
    printf("Enter Name (string, max 20 characters): ");
    scanf("%s", name);
    MAKE_STRING_VALUE(nameVal, name);

    printf("Enter Age (integer): ");
    scanf("%d", &age);
    MAKE_VALUE(ageVal, DT_INT, age);

    // Set attributes of the record
    setAttr(record, rel->schema, 0, idVal); // Set ID
    setAttr(record, rel->schema, 1, nameVal); // Set Name
    setAttr(record, rel->schema, 2, ageVal); // Set Age

    // Insert the record into the table
    RC result = insertRecord(rel, record);
    if (result == RC_OK) {
        printf("Record inserted successfully with ID %d.\n", nextID);
        nextID++; // Increment nextID to ensure unique IDs
    } else {
        printf("Failed to insert record. Error code: %d\n", result);
    }

    // Free allocated memory for values and record
    freeVal(idVal);
    freeVal(nameVal);
    freeVal(ageVal);
    freeRecord(record);
}


void delete_record(RM_TableData *rel) {
    int page, slot;
    printf("Enter Page Number: ");
    scanf("%d", &page);
    printf("Enter Slot Number: ");
    scanf("%d", &slot);
    RID rid = {page, slot};

    RC result = deleteRecord(rel, rid);
    if (result == RC_OK) {
        printf("Record deleted successfully.\n");
    } else {
        printf("Failed to delete record. Error code: %d\n", result);
    }
}

void update_record(RM_TableData *rel) {
    int page, slot;
    printf(" -- Updates selected record with data {Id: 1, Name: Alex, Age: 35}\n");
    printf(" -- To view record page and slot, go back and chose option '6'\n");
    printf("Enter Page Number of the record to update: ");
    scanf("%d", &page);
    printf("Enter Slot Number: ");
    scanf("%d", &slot);
    RID rid = {page, slot};

    Record *record;
    createRecord(&record, rel->schema);
    record->id = rid;

    Value *idVal, *nameVal, *ageVal;
    MAKE_VALUE(idVal, DT_INT, 1);
    MAKE_STRING_VALUE(nameVal, "Alex");
    MAKE_VALUE(ageVal, DT_INT, 35);

    setAttr(record, rel->schema, 0, idVal);
    setAttr(record, rel->schema, 1, nameVal);
    setAttr(record, rel->schema, 2, ageVal);

    RC result = updateRecord(rel, record);
    if (result == RC_OK) {
        printf("Record updated successfully.\n");
    } else {
        printf("Failed to update record. Error code: %d\n", result);
    }

    freeVal(idVal);
    freeVal(nameVal);
    freeVal(ageVal);
    freeRecord(record);
}

void view_records(RM_TableData *rel) {
    RM_ScanHandle scan;
    Expr *condition = NULL; // NULL condition means scan all records
    startScan(rel, &scan, condition);

    Record *record;
    createRecord(&record, rel->schema);
    Value *idVal, *nameVal, *ageVal;

    printf("Current records in table:\n");
    printf("Page\tSlot\tID\tName\t\tAge\n");

    while (next(&scan, record) != RC_RM_NO_MORE_TUPLES) {
        // Retrieve each attribute to validate and display the record
        getAttr(record, rel->schema, 0, &idVal);
        getAttr(record, rel->schema, 1, &nameVal);
        getAttr(record, rel->schema, 2, &ageVal);

        // Display the record's page, slot, ID, Name, and Age if valid
        if (strlen(nameVal->v.stringV) > 0) {
            printf("%d\t%d\t%d\t%s\t%d\n",
                   record->id.page,
                   record->id.slot,
                   idVal->v.intV,
                   nameVal->v.stringV,
                   ageVal->v.intV);
        }

        // Free each attribute value after use to prevent memory leaks
        freeVal(idVal);
        freeVal(nameVal);
        freeVal(ageVal);
    }

    closeScan(&scan);
    freeRecord(record);
}



void close_table(RM_TableData *rel) {
    RC result = closeTable(rel);
    if (result == RC_OK) {
        printf("Table closed successfully.\n");
    } else {
        printf("Failed to close table. Error code: %d\n", result);
    }
}

int main() {
    RM_TableData table;
    char tableName[50];
    int choice;

    initRecordManager(NULL);

    while (1) {
        print_menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter table name: ");
                scanf("%s", tableName);
                create_table(tableName);
                break;
            case 2:
                printf("Enter table name to open: ");
                scanf("%s", tableName);
                open_table(&table, tableName);
                break;
            case 3:
                insert_record(&table);
                break;
            case 4:
                delete_record(&table);
                break;
            case 5:
                update_record(&table);
                break;
            case 6:
                view_records(&table);
                break;
            case 7:
                close_table(&table);
                break;
            case 8:
                shutdownRecordManager();
                printf("Exiting Record Manager CLI.\n");
                exit(0);
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
}
