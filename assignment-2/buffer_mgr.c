#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dberror.h"
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "storage_mgr.h"


RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		  const int numPages, ReplacementStrategy strategy,
		  void *stratData)
{
  bm->pageFile = (char *)pageFileName;
  bm->numPages = numPages;
  bm->strategy = strategy;

  bm->mgmtData = malloc(sizeof(BM_MgmtData));
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  mgmt->frames = (BM_PageHandle *)malloc(numPages * sizeof(BM_PageHandle));
  mgmt->fifoQueue = (int *)malloc(numPages * sizeof(int));

  for (int i = 0; i < numPages; i++) {
    mgmt->frames[i].pageNum = NO_PAGE;
    mgmt->frames[i].data = NULL;
    mgmt->fifoQueue[i] = i;
  }

  mgmt->fifoQueue = (int *)malloc(numPages * sizeof(int));
  for (int i = 0; i < numPages; i++) {
    mgmt->fifoQueue[i] = i;
  }

  mgmt->readIO = 0;
  mgmt->writeIO = 0;
  mgmt->dirtyFlags = (bool *)calloc(numPages, sizeof(bool));
  mgmt->fixCounts = (int *)calloc(numPages, sizeof(int));

  mgmt->timestamps = (int *)calloc(numPages, sizeof(int));
  mgmt->currentTimestamp = 0;

  SM_FileHandle fileHandle;
  RC rc = openPageFile(pageFileName, &fileHandle);
  if (rc != RC_OK) {
    return rc;
  }
  mgmt->fileHandle = fileHandle;

  return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm) {
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  // Loop through all frames
  for (int i = 0; i < bm->numPages; i++) {
    // Check if the frame has been allocated and is dirty
    if (mgmt->frames[i].pageNum != NO_PAGE) {
      if (mgmt->dirtyFlags[i] == true) {
        // Force the page to write back if it is dirty
        if (mgmt->fixCounts[i] > 0) {
          return RC_PINNED_PAGES;  // Cannot shutdown if there are pinned pages
        }
        forcePage(bm, &mgmt->frames[i]);
      }
      // Free the data in the frame to avoid memory leaks
      free(mgmt->frames[i].data);
      mgmt->frames[i].data = NULL; // Set to NULL to avoid dangling pointer
    }
  }

  // Free management arrays
  free(mgmt->frames);
  free(mgmt->dirtyFlags);
  free(mgmt->fixCounts);
  free(mgmt->fifoQueue);
  free(mgmt->timestamps);

  // Close the page file
  RC rc = closePageFile(&(mgmt->fileHandle));
  if (rc != RC_OK) {
    return rc;
  }

  // Free the management data and the buffer pool structure
  free(mgmt);
  // free(bm);

  return RC_OK;
}


RC forceFlushPool(BM_BufferPool *const bm)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  for (int i = 0; i < bm->numPages; i++) {
    if (mgmt->frames[i].pageNum != NO_PAGE && mgmt->dirtyFlags[i] == true
        && mgmt->fixCounts[i] == 0) {
      forcePage(bm, &mgmt->frames[i]);
      mgmt->dirtyFlags[i] = false;
    }
  }

  return RC_OK;
}

RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  for (int i = 0; i < bm->numPages; i++) {
    if (mgmt->frames[i].pageNum == page->pageNum) {
      mgmt->dirtyFlags[i] = true;
      return RC_OK;
    }
  }

  return RC_PAGE_NOT_FOUND;
}

// RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
// {
//   BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
//
//   for (int i = 0; i < bm->numPages; i++) {
//     if (mgmt->frames[i].pageNum == page->pageNum) {
//       if (mgmt->fixCounts[i] == 0)
//         return RC_PAGE_NOT_PINNED;
//
//       mgmt->fixCounts[i]--;
//       return RC_OK;
//     }
//   }
//
//   return RC_PAGE_NOT_FOUND;
// }

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  for (int i = 0; i < bm->numPages; i++) {
    if (mgmt->frames[i].pageNum == page->pageNum) {
      if (mgmt->dirtyFlags[i] == true) {
        writeBlock(page->pageNum, &(mgmt->fileHandle), mgmt->frames[i].data);
        mgmt->dirtyFlags[i] = false;
        mgmt->writeIO++;
      }
      return RC_OK;
    }
  }

  return RC_PAGE_NOT_FOUND;
}

static int findFrameToReplace(BM_BufferPool *const bm) {
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  if (bm->strategy == RS_FIFO) {
    // Implement FIFO strategy
    for (int i = 0; i < bm->numPages; i++) {
      int frameIndex = mgmt->fifoQueue[i];
      if (mgmt->fixCounts[frameIndex] == 0) {
        // Move this frame to the end of the queue
        for (int j = i; j < bm->numPages - 1; j++) {
          mgmt->fifoQueue[j] = mgmt->fifoQueue[j + 1];
        }
        mgmt->fifoQueue[bm->numPages - 1] = frameIndex;
        return frameIndex;
      }
    }
  } else if (bm->strategy == RS_LRU) {
    int leastUsed = mgmt->timestamps[0];
    int leastUsedIndex = 0;
    for (int i = 1; i < bm->numPages; i++) {
      if (mgmt->fixCounts[i] == 0 && mgmt->timestamps[i] < leastUsed) {
        leastUsed = mgmt->timestamps[i];
        leastUsedIndex = i;
      }
    }
    return leastUsedIndex;
  }

  return NO_PAGE;
}

static void updateLRUOrder(BM_BufferPool *const bm, int frameIndex) {
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
  mgmt->timestamps[frameIndex] = ++(mgmt->currentTimestamp);
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

    // Check if the page is already in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (mgmt->frames[i].pageNum == pageNum) {
            page->pageNum = pageNum;
            page->data = mgmt->frames[i].data;
            mgmt->fixCounts[i]++;
            if (bm->strategy == RS_LRU) {
                updateLRUOrder(bm, i);
            }
            return RC_OK;
        }
    }

    // If the page is not in the buffer pool, we need to load it
    int frameIndex = findFrameToReplace(bm);
    if (frameIndex == NO_PAGE)
        return -1;

    // If the chosen frame is dirty, write it back to disk
    if (mgmt->dirtyFlags[frameIndex]) {
        forcePage(bm, &mgmt->frames[frameIndex]);
    }

    // Load the new page from disk
    if (mgmt->frames[frameIndex].data != NULL) {
        free(mgmt->frames[frameIndex].data);
    }
    mgmt->frames[frameIndex].data = malloc(PAGE_SIZE);
    RC rc = readBlock(pageNum, &(mgmt->fileHandle), mgmt->frames[frameIndex].data);
    if (rc != RC_OK) {
        if (rc == RC_READ_NON_EXISTING_PAGE) {
            // Initialize new page
            memset(mgmt->frames[frameIndex].data, 0, PAGE_SIZE);
            char pageContent[PAGE_SIZE];
            snprintf(pageContent, PAGE_SIZE, "Page-%i", pageNum);
            strncpy(mgmt->frames[frameIndex].data, pageContent, strlen(pageContent));
        } else {
            return rc;
        }
    }

    mgmt->frames[frameIndex].pageNum = pageNum;
    mgmt->fixCounts[frameIndex] = 1;
    mgmt->dirtyFlags[frameIndex] = false;
    mgmt->readIO++;

    if (bm->strategy == RS_LRU) {
        updateLRUOrder(bm, frameIndex);
    }

    page->pageNum = pageNum;
    page->data = mgmt->frames[frameIndex].data;
    return RC_OK;
}

// Modify the unpinPage function
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
    for (int i = 0; i < bm->numPages; i++) {
        if (mgmt->frames[i].pageNum == page->pageNum) {
            if (mgmt->fixCounts[i] == 0)
                return RC_PAGE_NOT_PINNED;
            mgmt->fixCounts[i]--;
            if (bm->strategy == RS_LRU) {
                updateLRUOrder(bm, i);
            }
            return RC_OK;
        }
    }
    return RC_PAGE_NOT_FOUND;
}

PageNumber *getFrameContents(BM_BufferPool *const bm)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
  PageNumber *frameContents = malloc(bm->numPages * sizeof(PageNumber));

  for (int i = 0; i < bm->numPages; i++)
    frameContents[i] = mgmt->frames[i].pageNum;

  return frameContents;
}

bool *getDirtyFlags(BM_BufferPool *const bm)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
  bool *dirtyFlags = malloc(bm->numPages * sizeof(bool));

  for (int i = 0; i < bm->numPages; i++)
    dirtyFlags[i] = mgmt->dirtyFlags[i];

  return dirtyFlags;
}

int *getFixCounts(BM_BufferPool *const bm)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
  int *fixCounts = malloc(bm->numPages * sizeof(int));

  for (int i = 0; i < bm->numPages; i++)
    fixCounts[i] = mgmt->fixCounts[i];

  return fixCounts;
}

int getNumReadIO(BM_BufferPool *const bm)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
  return mgmt->readIO;
}

int getNumWriteIO(BM_BufferPool *const bm)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;
  return mgmt->writeIO;
}
