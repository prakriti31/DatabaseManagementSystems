#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * ############################
 *    Page File Manipulation
 * ############################
 */

void initStorageManager(void) {
    printf("Storage manager init.\n");
}
// Kamakshya Nanda
RC createPageFile(char *fileName) {
    /* 
     * This function creates a File first,
     * then writes '/0' byte to it, to create a page.
     */
    FILE *file = fopen(fileName, "w");   // "w" creates file if it doesn't exist and truncates the file if it exists
    if (file == NULL) {
        printf("New file initiated");
    }
    for (int i = 0; i < PAGE_SIZE; i++) {
        fputc('\0', file);
    }
    fclose(file);
    return RC_OK;


}

// Yash Vardhan Sharma
RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    /*
     * This function is used to open the file created
     * by the createPageFile function
     */

    // Try opening the file in read binary mode
    FILE *page = fopen(fileName, "rb");
    // perror("fopen");
    if (page == NULL) {
        printf("Error: File '%s' not found.\n", fileName);
        return RC_FILE_NOT_FOUND;  // Return "file not found" if open fails
    }
    else {
        // File opened successfully
        fseek(page, 0, SEEK_END);
        long fileSize = ftell(page);
        int no_of_pages = fileSize / PAGE_SIZE;
        fHandle -> fileName = fileName;
        fHandle -> curPagePos = 0;
        fHandle -> totalNumPages = no_of_pages;
        fHandle -> mgmtInfo = page;
        return RC_OK;
    }
}

// Yash Vardhan Sharma
RC closePageFile(SM_FileHandle *fHandle) {
    // Check if SM_FileHandle -> fileName is not empty to validate that file was opened.

    if(fHandle -> fileName == NULL) {
        printf("Error: File '%s' not found.\n", fHandle -> fileName);
    }
    fclose(fHandle -> mgmtInfo);
    return RC_OK;
}

// Prakriti Sharma
RC destroyPageFile(char *fileName) {
    if(remove(fileName) == 0) return RC_OK;
    return RC_FILE_NOT_FOUND;

}

/*
 * ############################
 *   Reading blocks from disk
 * ############################
 */

// Kamakshya, Yash and Prakriti
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

    // This function reads a block/page from the file.

    FILE *file = fopen(fHandle -> fileName, "rb");

    if (file == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    if(pageNum > fHandle -> totalNumPages || pageNum < 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }

        /*
         * SEEK_SET = 0 (default)
         * Offset is where you want to take the seek to.
         * Eg: pageNum = 2, page starts from (2 - 1) * 4096 and ends at 2 * 4096
         * This is explained more briefly in readme file.
         */

    fseek(file, (pageNum) * PAGE_SIZE, SEEK_SET);
    fread(memPage, sizeof(char), PAGE_SIZE, file);
    fHandle -> curPagePos = pageNum;
    fclose(file);
    return RC_OK;
}

// Kamakshya, Yash and Prakriti
int getBlockPos (SM_FileHandle *fHandle) {
    // Returns current block/page position. (last accessed)
    return fHandle -> curPagePos;
}
// Kamakshya, Yash and Prakriti
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Reads 1st block/page from the file.
    readBlock(1,fHandle,memPage);
    return RC_OK;
}
// Kamakshya, Yash and Prakriti
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Reads block/page previous to the current block/page.
    readBlock(fHandle -> curPagePos - 1,fHandle,memPage);
    return RC_OK;
}
// Kamakshya, Yash and Prakriti
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Reads current block/page from the file.
    readBlock(fHandle -> curPagePos,fHandle,memPage);
    return RC_OK;
}
// Kamakshya, Yash and Prakriti
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Reads block/page next to the current block/page.
    readBlock(fHandle -> curPagePos + 1,fHandle,memPage);
    return RC_OK;
}
// Kamakshya, Yash and Prakriti
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Reads last block/page from the file.
    readBlock(fHandle -> totalNumPages,fHandle,memPage);
    return RC_OK;
}

/*
 * #################################
 *   Writing blocks to a page file
 * #################################
 */

// Kamakshya, Yash and Prakriti
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Writes a block/page on the file.
    FILE *file = fopen(fHandle -> fileName, "r+b");
    fseek(file, (pageNum) * PAGE_SIZE, SEEK_SET);
    fwrite(memPage, sizeof(char), PAGE_SIZE, file);
    fHandle -> curPagePos = pageNum;
    return RC_OK;
}
// Kamakshya, Yash and Prakriti
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Writes data to the current block/page.
    writeBlock(fHandle -> curPagePos, fHandle, memPage);
    return RC_OK;
}
// Kamakshya, Yash and Prakriti
RC appendEmptyBlock (SM_FileHandle *fHandle) {
    // Appends an empty block/page
    FILE *file = fopen(fHandle -> fileName, "wb");
    fHandle -> curPagePos = fHandle -> totalNumPages;
    fseek(file, fHandle -> totalNumPages * PAGE_SIZE, SEEK_SET);
    for (int i = 0; i < PAGE_SIZE; i++) {
        fputc('\0', file);
    }
    fHandle -> totalNumPages++;

    return RC_OK;
}
// Kamakshya, Yash and Prakriti
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
    // Ensures that total pages/blocks in the file is up to date with the number of pages in the struct.
    int totalPages = fHandle -> totalNumPages;
    if (totalPages < numberOfPages) {
        int difference = numberOfPages - totalPages;
        if(difference > 0) {
            for(int i=0; i < difference; i++) {
                appendEmptyBlock(fHandle);
            }
        }

    }
    return RC_OK;
}