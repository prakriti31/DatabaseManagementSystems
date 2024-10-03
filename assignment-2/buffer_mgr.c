#include "buffer_mgr.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "dberror.h"
#include "storage_mgr.h"

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
    if (frames == NULL) { // Check for successful allocation
        perror("Error: Memory allocation for frames failed.");
        return RC_FILE_NOT_FOUND; // Return an error code if allocation fails
    }

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

    // Check if any pages are pinned
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].fixCount > 0) {
            // If any page is pinned, return an error
            return RC_FILE_NOT_FOUND;
        }
    }

    // Write back dirty pages to disk before shutting down
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].isDirty) {
            // Write the page back to disk
            RC result = writeBlock(frames[i].pageNum, bm->pageFile, frames[i].data);
            if (result != RC_OK) {
                return result; // Return error if writing to disk fails
            }
        }
    }

    // Free the allocated memory for frames
    free(frames);

    // Free the page file name if it was duplicated
    free(bm->pageFile);

    // Clear the pointer
    bm->mgmtData = NULL;

    return RC_OK; // Return success code
}

RC forceFlushPool(BM_BufferPool *const bm) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the pointer to frames

    // Iterate over all frames in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        // Check if the frame is dirty and has a fix count of 0
        if (frames[i].isDirty && frames[i].fixCount == 0) {
            // Create a temporary page handle to pass to forcePage
            BM_PageHandle pageHandle = {.pageNum = frames[i].pageNum, .data = frames[i].data};

            // Write the page to disk
            RC writeStatus = forcePage(bm, &pageHandle);
            if (writeStatus != RC_OK) {
                return writeStatus; // Return error code if writing to disk fails
            }

            // Mark the page as clean since it's written to disk
            frames[i].isDirty = false;
        }
    }

    return RC_OK; // Return success code
}


// Buffer Manager Interface Access Pages
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the pointer to frames

    // Iterate over all frames to find the one corresponding to the page
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].pageNum == page->pageNum) { // Check if the page number matches
            frames[i].isDirty = true; // Mark the page as dirty
            return RC_OK; // Return success code
        }
    }

    // If the page is not found in any frame, return an error
    return RC_message("Page not found.");
}

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the pointer to frames

    // Iterate over all frames to find the one corresponding to the page
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].pageNum == page->pageNum) { // Check if the page number matches
            // Check if the fixCount is greater than 0 before decrementing
            if (frames[i].fixCount > 0) {
                frames[i].fixCount--; // Decrement the fix count
                return RC_OK; // Return success code
            } else {
                // If the fix count is already 0, return an error (optional, depending on requirements)
                return RC_message(""); // You can define this error code in dberror.h
            }
        }
    }

    // If the page is not found in any frame, return an error
    return RC_message("");
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the pointer to frames

    // Iterate over all frames to find the one corresponding to the page
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].pageNum == page->pageNum) { // Check if the page number matches
            // Check if the page is dirty
            if (frames[i].isDirty) {
                // Write the page content to disk
                RC writeStatus = writeBlock(frames[i].pageNum, bm->pageFile, frames[i].data);
                if (writeStatus != RC_OK) {
                    return writeStatus; // Return error code if writing to disk fails
                }

                // Mark the page as clean since it's now written to disk
                frames[i].isDirty = false;
            }
            return RC_OK; // Return success code
        }
    }

    // If the page is not found in any frame, return an error
    return RC_message("");
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