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
    Frame *frames = (Frame *)bm->mgmtData; // Get the frames stored in mgmtData

    // Before shutting down, ensure that all pinned pages are unpinned
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].fixCount > 0) {
            // return RC_message("RC_SHUTDOWN_POOL_HAS_PINNED_PAGES"); // Error: Pool has pinned pages
            return RC_FILE_NOT_FOUND;
        }
    }

    // Flush all dirty pages to disk
    RC status = forceFlushPool(bm);
    if (status != RC_OK) {
        return status; // Return error code if forceFlushPool fails
    }

    // Free the memory allocated for the frames array
    free(frames);
    bm->mgmtData = NULL; // Set the management data to NULL

    // Free the page file name
    free(bm->pageFile);
    bm->pageFile = NULL;

    return RC_OK; // Success
}

RC forceFlushPool(BM_BufferPool *const bm) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the frames stored in mgmtData

    SM_FileHandle fh;  // Declare an SM_FileHandle for the file

    // Open the page file using the Storage Manager function
    RC status = openPageFile(bm->pageFile, &fh);
    if (status != RC_OK) {
        return status; // Return error code if file opening fails
    }

    // Iterate through all the frames
    for (int i = 0; i < bm->numPages; i++) {
        // Check if the page is dirty and not pinned (fixCount == 0)
        if (frames[i].isDirty && frames[i].fixCount == 0) {
            // Write the dirty page back to disk using the SM_FileHandle
            status = writeBlock(frames[i].pageNum, &fh, frames[i].data);
            if (status != RC_OK) {
                return status; // Return error code if writeBlock fails
            }

            // Mark the page as clean after writing to disk
            frames[i].isDirty = false;

            // Increment the write I/O count
            bm->numWriteIO++; // Increment the number of write I/O operations
        }
    }

    // Close the page file after writing all dirty pages
    closePageFile(&fh);

    return RC_OK; // Success
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
    // return RC_message("Page not found.");
    return RC_FILE_NOT_FOUND;
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
                // return RC_message("RC_PIN_COUNT_ERROR"); // You can define this error code in dberror.h
                return RC_FILE_NOT_FOUND;
            }
        }
    }

    // If the page is not found in any frame, return an error
    // return RC_message("RC_PAGE_NOT_FOUND");
    return RC_FILE_NOT_FOUND;
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the frames stored in mgmtData
    SM_FileHandle fh; // Declare an SM_FileHandle

    // Open the page file using the Storage Manager function
    RC status = openPageFile(bm->pageFile, &fh);
    if (status != RC_OK) {
        return status; // Return error code if file opening fails
    }

    // Iterate through the frames to find the page
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].pageNum == page->pageNum) { // Check if the page number matches
            // Write the page back to disk using the SM_FileHandle
            status = writeBlock(frames[i].pageNum, &fh, frames[i].data);
            if (status != RC_OK) {
                closePageFile(&fh); // Close the file handle before returning
                return status; // Return error code if writeBlock fails
            }
            if (status == RC_OK) {
                bm->numWriteIO++; // Increment the write I/O count
            }

            // Mark the page as clean after writing to disk
            frames[i].isDirty = false;

            // Increment the write I/O count
            bm->numWriteIO++; // Increment the number of write I/O operations

            closePageFile(&fh); // Close the file handle after writing
            return RC_OK; // Return success
        }
    }

    // Close the page file if the page was not found in the buffer pool
    closePageFile(&fh);
    // return RC_message("RC_PAGE_NOT_FOUND"); // If the page was not found
    return RC_FILE_NOT_FOUND;
}


RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the frames stored in mgmtData
    SM_FileHandle fh; // Declare an SM_FileHandle

    // Open the page file using the Storage Manager function
    RC status = openPageFile(bm->pageFile, &fh);
    if (status != RC_OK) {
        return status; // Return error code if file opening fails
    }

    // Check if the requested page is already in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].pageNum == pageNum) { // Page is already in the buffer
            frames[i].fixCount++; // Increment the fix count
            page->pageNum = frames[i].pageNum; // Set the page number in the page handle
            page->data = frames[i].data; // Set the data pointer to the frame's data
            closePageFile(&fh); // Close the file handle before returning
            return RC_OK; // Return success
        }
    }

    // If the page is not in the buffer, we need to load it
    // Find a frame to replace (evict) if necessary
    int frameToReplace = -1;

    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].fixCount == 0) { // Look for a frame that is not pinned
            frameToReplace = i; // This frame can be replaced
            break;
        }
    }

    // If no unpinned frames are available, we need to evict a frame
    if (frameToReplace == -1) {
        // Implement your page replacement strategy here (e.g., FIFO, LRU)
        // For now, we will just use a simple approach: evict the first frame
        // In a real implementation, you should use the specified replacement strategy.
        frameToReplace = 0; // Just evict the first frame (for demonstration purposes)
    }

    // If the frame to replace has a dirty page, write it back to disk
    if (frames[frameToReplace].isDirty) {
        status = writeBlock(frames[frameToReplace].pageNum, &fh, frames[frameToReplace].data);
        if (status != RC_OK) {
            closePageFile(&fh); // Close the file handle before returning
            return status; // Return error code if writeBlock fails
        }
    }

    // Load the new page from disk into the frame
    status = readBlock(pageNum, &fh, frames[frameToReplace].data);
    if (status != RC_OK) {
        closePageFile(&fh); // Close the file handle before returning
        return status; // Return error code if readBlock fails
    }
    if (status == RC_OK) {
        bm->numReadIO++; // Increment the read I/O count
    }


    // Update the frame with the new page information
    frames[frameToReplace].pageNum = pageNum; // Update the page number in the frame
    frames[frameToReplace].isDirty = false; // The page is initially clean
    frames[frameToReplace].fixCount = 1; // Set the fix count to 1 for the new page

    // Set the page handle to point to the loaded page
    page->pageNum = pageNum; // Set the page number in the page handle
    page->data = frames[frameToReplace].data; // Set the data pointer to the new frame's data

    closePageFile(&fh); // Close the file handle after processing
    return RC_OK; // Return success
}


// Statistics Interface
PageNumber *getFrameContents(BM_BufferPool *const bm) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the frames stored in mgmtData
    PageNumber *frameContents = malloc(bm->numPages * sizeof(PageNumber)); // Allocate memory for the output array

    if (frameContents == NULL) {
        return NULL; // Return NULL if memory allocation fails
    }

    // Iterate through the frames and fill the output array
    for (int i = 0; i < bm->numPages; i++) {
        frameContents[i] = frames[i].pageNum; // Set the page number for the frame
    }

    return frameContents; // Return the array of page numbers
}


bool *getDirtyFlags(BM_BufferPool *const bm) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the frames stored in mgmtData
    bool *dirtyFlags = malloc(bm->numPages * sizeof(bool)); // Allocate memory for the output array

    if (dirtyFlags == NULL) {
        return NULL; // Return NULL if memory allocation fails
    }

    // Iterate through the frames and fill the output array
    for (int i = 0; i < bm->numPages; i++) {
        dirtyFlags[i] = frames[i].isDirty; // Set the dirty flag for the frame
    }

    return dirtyFlags; // Return the array of dirty flags
}

int *getFixCounts(BM_BufferPool *const bm) {
    Frame *frames = (Frame *)bm->mgmtData; // Get the frames stored in mgmtData
    int *fixCounts = malloc(bm->numPages * sizeof(int)); // Allocate memory for the output array

    if (fixCounts == NULL) {
        return NULL; // Return NULL if memory allocation fails
    }

    // Iterate through the frames and fill the output array
    for (int i = 0; i < bm->numPages; i++) {
        fixCounts[i] = frames[i].fixCount; // Set the fix count for the frame
    }

    return fixCounts; // Return the array of fix counts
}

int getNumReadIO(BM_BufferPool *const bm) {
    return bm->numReadIO; // Return the number of read I/O operations
}

int getNumWriteIO(BM_BufferPool *const bm) {
    return bm->numWriteIO; // Return the number of write I/O operations
}