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
    printf("Storage manager init\n");
}
RC createPageFile(char *fileName) {
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

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    // Try opening the file in read binary mode
    FILE *page = fopen(fileName, "rb");
    perror("fopen");
    if (page == NULL) {
        printf("Error: File '%s' not found.\n", fileName);
        return RC_FILE_NOT_FOUND;  // Return "file not found" if open fails
    }
    else {
        // File opened successfully
        // printf("File '%s' opened successfully.\n", fileName);
        fseek(page, 0, SEEK_END);
        long fileSize = ftell(page);
        // printf("File size is %ld\n", fileSize);
        int no_of_pages = fileSize / PAGE_SIZE;
        // printf("Number of pages = %d\n", no_of_pages);
        fHandle -> fileName = fileName;
        fHandle -> curPagePos = 0;
        fHandle -> totalNumPages = no_of_pages;
        fHandle -> mgmtInfo = page;
        // printf("Current filename is %s\n", fHandle -> fileName);
        // printf("Current curPagePos is %d\n", fHandle -> curPagePos);
        // printf("Current totalNumPages is %d\n", fHandle -> totalNumPages);
        return RC_OK;
    }
}

RC closePageFile(SM_FileHandle *fHandle) {
    printf("Inside closePageFile()\n");

    // Check if SM_FileHandle -> fileName is not empty to validate that file was opened.

    if(fHandle -> fileName == NULL) {
        printf("Error: File '%s' not found.\n", fHandle -> fileName);
    }

    // Open file before closing it :/
    // mode = 'rb' and not 'r' because the file is a .bin file
    // 'r' is used to read text files, 'rb' for rest of the files.

    // FILE *file = fopen(fHandle -> fileName, "rb");
    // if(file == NULL) {
    //     return RC_FILE_NOT_FOUND;
    // }
    // if(fclose(file) !=0 ) {
    //     RC_message = "Error closing file";
    //     return RC_message;
    // }
    // fclose(fHandle -> fileName);
    fclose(fHandle -> mgmtInfo);
    perror("fclose");
    return RC_OK;
}

RC destroyPageFile(char *fileName) {
    if(remove(fileName) == 0) return RC_OK;
    perror("Error destroying file");
    return RC_FILE_NOT_FOUND;

}

/*
 * ############################
 *   Reading blocks from disk
 * ############################
 */

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

    FILE *file = fopen(fHandle -> fileName, "rb");

    if (file == NULL) {
        // printf("Error: File '%s' not found.\n", fHandle -> fileName);
        return RC_FILE_NOT_FOUND;
    }
    else {
        if(pageNum > fHandle -> totalNumPages || pageNum < 0) {
            return RC_READ_NON_EXISTING_PAGE;
        }
        // SEEK_SET = 0
        // Offset is where you want to take the seek to.
        // Eg: pageNum = 2
        // page starts from (2 - 1) * 4096 and ends at 2 * 4096
        else {
            fseek(file, (pageNum - 1) * PAGE_SIZE, SEEK_SET);
            // ftell(file); // Get current pointer position.
            fread(memPage, sizeof(char), PAGE_SIZE, file);
            fHandle -> curPagePos = pageNum;
            return RC_OK;
        }
    }
}

int getBlockPos (SM_FileHandle *fHandle) {
    return fHandle -> curPagePos;
}
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    readBlock(1,fHandle,memPage);
    return RC_OK;
}
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    readBlock(fHandle -> curPagePos - 1,fHandle,memPage);
    return RC_OK;
}
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    readBlock(fHandle -> curPagePos,fHandle,memPage);
}
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    readBlock(fHandle -> curPagePos + 1,fHandle,memPage);
    return RC_OK;
}
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    readBlock(fHandle -> totalNumPages,fHandle,memPage);
    return RC_OK;
}

/*
 * #################################
 *   Writing blocks to a page file
 * #################################
 */

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    FILE *file = fopen(fHandle -> fileName, "wb");
    fseek(file, (pageNum - 1) * PAGE_SIZE, SEEK_SET);
    fwrite(memPage, sizeof(char), PAGE_SIZE, file);
    fHandle -> curPagePos = pageNum;
    return RC_OK;
}
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    writeBlock(fHandle -> curPagePos, fHandle, memPage);
    return RC_OK;
}
RC appendEmptyBlock (SM_FileHandle *fHandle) {
    FILE *file = fopen(fHandle -> fileName, "wb");

    fHandle -> curPagePos = fHandle -> totalNumPages;
    fseek(file, fHandle -> totalNumPages * PAGE_SIZE, SEEK_SET);
    for (int i = 0; i < PAGE_SIZE; i++) {
        fputc('\0', file);
    }
    fHandle -> totalNumPages++;

    return RC_OK;
}
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
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