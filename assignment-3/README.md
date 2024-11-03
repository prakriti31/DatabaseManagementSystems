# Record Manager - Programming Assignment 3

This project implements a simple Record Manager as part of CS525 Advanced Database Organization. The Record Manager allows for operations such as record insertion, deletion, updating, and scanning records within a table. It utilizes a fixed schema per table and employs a page-based storage approach via the Buffer Manager, developed in previous assignments.

## Contributors

- **Yash**: Table and Record Manager Initialization, Table Creation and Management
- **Prakriti**: Record Insertion, Deletion, Updating, Retrieval
- **Kamakshya**: Scanning Operations, Schema and Attribute Management, and Record Size Calculation

## File Structure

- `record_mgr.c` - Core implementation of Record Manager functions.
- `buffer_mgr.c` - Buffer Manager for handling page files (from previous assignment).
- `storage_mgr.c` - Storage Manager for basic page operations (from previous assignment).
- `expr.c` - Expressions for record scanning conditions.
- `rm_serializer.c` - Serialization functions (provided).
- `test_assign3_1.c` - Test cases for Record Manager functionality.
- `CMakeLists.txt` - Build configurations for the project.

## Setup and Build Instructions

### Prerequisites

Ensure you have the following installed:
- GCC or any C compiler
- CMake for build management

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://bitbucket.org/fall_24/fall_2024_32/src/master/
   cd assignment-3
   ```
2. Use CMake to configure and build the project:
   ```bash
       mkdir build
       cd build
       cmake ..
       make
   ```
3. Running Tests
   After building, you can run the test cases to verify the functionality of the Record Manager.
   ```bash
   ./test_assign3_1
   ```

This command will execute the tests and output results, verifying core functionalities such as table creation, record operations, and scans.

### Overview of Key Functions & Member Contributions
1. Table and Record Manager Functions (_**Yash Vardhan Sharma**_)

   - `initRecordManager`: Initializes the Record Manager.
   - `shutdownRecordManager`: Shuts down the Record Manager.
   - `createTable`: Creates a table with the specified schema.
   - `openTable`: Opens a table for operations.
   - `closeTable`: Closes a table.
   - `deleteTable`: Deletes a table.
   - `getNumTuples`: Returns the number of tuples (records) in a table.


2. Record Operations (**_Prakriti Sharma_**)

   - `insertRecord`: Inserts a new record into a table and assigns an RID.
   - `deleteRecord`: Deletes a record by its RID.
   - `updateRecord`: Updates an existing record in the table.
   -  `getRecord`: Retrieves a record based on its RID.


3. Scan Functions (**_Kamakshya Nanda_**)

   - `startScan`: Initiates a scan operation on a table based on a specified condition.
   - `next`: Retrieves the next record that satisfies the scan condition.
   - `closeScan`: Closes the scan and releases resources.

 
4. Schema and Attribute Management (**_Kamakshya Nanda_**)

   - `getRecordSize`: Calculates the size of records based on the schema.
   - `createSchema`: Creates a schema for a table.
   - `freeSchema`: Frees memory associated with a schema.
   - `createRecord`: Allocates memory for a new record.
   - `getAttr`: Retrieves an attribute value from a record.
   - `setAttr`: Sets an attribute value in a record.