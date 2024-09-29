#include "buffer_mgr.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "dberror.h"

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData) {
    // Check if the page file exists
    if (access(pageFileName, F_OK) == -1) { // F_OK checks for existence
        perror("Error: Page file does not exist."); // Print an error message
        return RC_FILE_NOT_FOUND; // Return an error code
    }

    // Proceed with the initialization if the file exists
    bm->pageFile = strdup(pageFileName); // Copy the page file name
    bm->numPages = numPages;              // Set the number of pages
    bm->strategy = strategy;              // Set the replacement strategy

    // Allocate memory for the frames array
    Frame *frames = malloc(numPages * sizeof(Frame));
    // if (frames == NULL) { // Check for successful allocation
    //     perror("Error: Memory allocation for frames failed.");
    //     return RC_FILE_NOT_FOUND;// Return an error code if allocation fails
    // }

    // Initialize each frame
    for (int i = 0; i < numPages; i++) {
        frames[i].pageNum = NO_PAGE; // Initialize to indicate empty frame
        frames[i].isDirty = false;    // Initially not dirty
        frames[i].fixCount = 0;       // No clients using this page
        frames[i].data = NULL;        // Pointer to data is initially NULL
    }

    bm->mgmtData = frames; // Store the pointer to frames in mgmtData

    return RC_OK; // Return success code
}
RC shutdownBufferPool(BM_BufferPool *const bm) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the pointer to frames
    free(frames); // Free the allocated memory for frames
    free(bm->pageFile); // Free the page file name if it was duplicated
    bm->mgmtData = NULL; // Clear the pointer
    return RC_OK; // Return success code
}
RC forceFlushPool(BM_BufferPool *const bm) {
    return 0;
}

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page) {
    return 0;
}
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page) {
    return 0;
}
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page) {
    return 0;
}
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
        const PageNumber pageNum) {
    return 0;
}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm) {
    return NULL;
}
bool *getDirtyFlags (BM_BufferPool *const bm) {
    return NULL;
}
int *getFixCounts (BM_BufferPool *const bm) {
    return NULL;
}
int getNumReadIO (BM_BufferPool *const bm) {
    return 0;
}
int getNumWriteIO (BM_BufferPool *const bm) {
    return 0;
}