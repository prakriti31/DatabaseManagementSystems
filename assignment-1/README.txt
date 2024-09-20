
## Overview

This project implements a simple Storage Manager for database files. The storage manager allows reading and writing blocks (or pages) from a file on disk into memory and vice versa. The manager deals with fixed-size pages (4096 bytes) and provides several methods to create, open, and manage files efficiently.

### File and Page Structure

The system is designed to store data in files, which are divided into fixed-size pages or blocks. Each block is 4096 bytes (4 KB), and these blocks serve as the basic units of data manipulation.

#### Diagram:

+------------------------------------+
|                                    |
|              File                  |
|                                    |
|                                    |
|                                    |
|                                    |
+------------------------------------+
|  +-----------+   +-----------+     |
|  |  Page 1   |   |  Page 2   |     |
|  +-----------+   +-----------+     |
+------------------------------------+

- File: This is the logical container that holds multiple pages. It represents a table, index, or other database structure.
- Pages: Each file consists of fixed-size blocks of 4 KB. Pages are the smallest unit of reading and writing operations.

### Key Functionalities

1. Page Handling: The manager provides methods to create, open, close, and destroy files. Pages can be read, written, and appended dynamically.
2. Error Handling: The system returns appropriate error codes for various operations, such as reading non-existing pages or handling unavailable files.
3. File Metadata: The storage manager keeps track of the total number of pages in a file, the current page position, and manages internal details like file descriptors.

### Source Code Structure

The project contains the following files:

- storage_mgr.c: Core implementation of the storage manager interface.
- dberror.c: Definitions of error codes and handling functions.
- test_assign1_1.c: Test cases for the storage manager.
- Makefile: To compile and run the code.
- README.txt: This file, describing the implementation and structure.

### Functions

1) initStorageManager(): Initializes the storage manager. This should be called before any file operations.
2) createPageFile(char *fileName): Creates a new page file of one page filled with zeros.
3) openPageFile(char *fileName, SM_FileHandle *fHandle): Opens an existing page file and initializes the file handle.
4) closePageFile(SM_FileHandle *fHandle): Closes an open page file.
5) destroyPageFile(char *fileName): Deletes the specified page file.
6) readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage): Reads the block at the specified position into memory.
7) getBlockPos(SM_FileHandle *fHandle): Returns the current page position in the file.
8) readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage): Reads the first block of the file.
9) readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage): Reads the block before the current position.
10) readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage): Reads the block at the current position.
11) readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage): Reads the block after the current position.
12) readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage): Reads the last block of the file.
13) writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage): Writes the block at the specified position in the file.
14) writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage): Writes the current block to the file.
15) appendEmptyBlock(SM_FileHandle *fHandle): Appends an empty block (filled with zeros) to the file.
16) ensureCapacity(int numberOfPages, SM_FileHandle *fHandle): Ensures that the file has at least the specified number of pages.

### Added Test Cases

1) Test Case: testAppendEmptyPage
Purpose: To validate the ability to append an empty page to a file and ensure that it is appended correctly.

Steps:
Create a new page file using createPageFile() and open it using openPageFile().
Append an empty page using appendEmptyBlock().
Verify the total number of pages has increased from 1 to 2.
Read the second page using readBlock() and ensure that all bytes in the page are initialized to zero. Cleanup by closing and destroying the page file.

Expected Results:
Appending the empty block should increase the page count.
Reading the newly appended page should return zero bytes.

2) Test Case: testReadLastBlock
Purpose: To ensure that the last block in a multi-page file can be correctly read and that its contents match what was written.

Steps:
Create a new page file and open it.
Write a page of data ('A' characters) to the first page using writeBlock().
Append an empty block and write data ('B' characters) to it.
Read the last block using readLastBlock() and verify that the contents match the 'B' characters written earlier.
Cleanup by closing and destroying the page file.

Expected Results:
The last block should contain the data that was last written to it.
Content should match 'B' characters in the second page.

3) Test Case: testReadBeyondEOF
Purpose:
To validate that attempting to read beyond the end of the file returns an appropriate error.

Steps:
Create a new page file and open it.
Attempt to read from page 2 (index 2) in a file that only has one page.
Verify that the read operation fails and returns an error.
Cleanup by closing and destroying the page file.

Expected Results:
The system should return an error when attempting to read beyond the last page of the file.
Test Case: testWriteReadBlockAtPosition

Purpose:
To ensure that writing and reading at a specific position in a multi-page file works as expected, and that writing to one page does not affect others.

Steps:
Create a new page file and append two empty pages using appendEmptyBlock().
Write data ('C' characters) to the second page (index 2) using writeBlock().
Read back the second page and verify that its contents match the written data.
Verify that the first page remains unaffected and still contains zero bytes.
Cleanup by closing and destroying the page file.

Expected Results:
Data written to the second page should be correctly saved and retrievable.
The first page should remain unaffected and empty.

### Contributions

1) Kamakshya Nanda - createPageFile()
2) Yash Vardhan Sharma - openPageFile(),closePageFile()
3) Prakriti Sharma - destroyPageFile()
4) Collaborative work (Everyone worked together) -
   readBlock(), writeBlock(), appendEmptyBlock(), readLastBlock(), getBlockPos(),
   readFirstBlock(), readPreviousBlock(), readCurrentBlock(), readNextBlock(),
   writeCurrentBlock(), ensureCapacity()
5)Test-cases(Kamakshya Nanda & Prakriti Sharma): testReadBeyondEOF(), testAppendEmptyPage(), testReadLastBlock()

