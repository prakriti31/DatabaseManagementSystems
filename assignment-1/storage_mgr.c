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

RC createPageFile(char *fileName) {
    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        printf("Error creating file\n");
        return RC_FILE_NOT_FOUND;
    }
    for (int i = 0; i < PAGE_SIZE; i++) {
        fputc('\0', file);
    }
    fclose(file);
    return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    FILE *page = fopen(fileName, "rb+");
    if (page == NULL) {
        printf("Error: File '%s' not found.\n", fileName);
        return RC_FILE_NOT_FOUND;
    }
    fseek(page, 0, SEEK_END);
    long fileSize = ftell(page);
    fHandle->fileName = fileName;
    fHandle->curPagePos = 0;
    fHandle->totalNumPages = fileSize / PAGE_SIZE;
    fHandle->mgmtInfo = page;
    return RC_OK;
}

RC closePageFile(SM_FileHandle *fHandle) {
    if (fHandle->mgmtInfo != NULL) {
        fclose(fHandle->mgmtInfo);
        fHandle->mgmtInfo = NULL;
    }
    return RC_OK;
}

RC destroyPageFile(char *fileName) {
    return (remove(fileName) == 0) ? RC_OK : RC_FILE_NOT_FOUND;
}

/*
 * ############################
 *   Reading blocks from disk
 * ############################
 */

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle->mgmtInfo == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    fseek(fHandle->mgmtInfo, pageNum * PAGE_SIZE, SEEK_SET);
    fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
    fHandle->curPagePos = pageNum;
    return RC_OK;
}

int getBlockPos(SM_FileHandle *fHandle) {
    return fHandle->curPagePos;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

/*
 * #################################
 *   Writing blocks to a page file
 * #################################
 */

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle->mgmtInfo == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    fseek(fHandle->mgmtInfo, pageNum * PAGE_SIZE, SEEK_SET);
    fwrite(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
    fHandle->curPagePos = pageNum;
    return RC_OK;
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

RC appendEmptyBlock(SM_FileHandle *fHandle) {
    if (fHandle->mgmtInfo == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    fseek(fHandle->mgmtInfo, fHandle->totalNumPages * PAGE_SIZE, SEEK_SET);
    for (int i = 0; i < PAGE_SIZE; i++) {
        fputc('\0', fHandle->mgmtInfo);
    }
    fHandle->totalNumPages++;
    fHandle->curPagePos = fHandle->totalNumPages - 1;
    return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle->totalNumPages < numberOfPages) {
        int pagesToAdd = numberOfPages - fHandle->totalNumPages;
        for (int i = 0; i < pagesToAdd; i++) {
            appendEmptyBlock(fHandle);
        }
    }
    return RC_OK;
}
