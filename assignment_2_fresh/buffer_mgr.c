#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dberror.h"
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "storage_mgr.h"

// Custom error codes
#define RC_PINNED_PAGES                  10
#define RC_BUFFER_POOL_ALREADY_INIT     201
#define RC_BUFFER_POOL_NOT_INIT         202
#define RC_BUFFER_POOL_SHUTDOWN         203
#define RC_PINNED_PAGES_IN_POOL         204
#define RC_PAGE_NOT_FOUND               205
#define RC_PAGE_ALREADY_PINNED          206
#define RC_PAGE_NOT_PINNED              207
#define RC_BUFFER_POOL_FULL             208
#define RC_REPLACEMENT_STRATEGY_NOT_IMPLEMENTED 209

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

  mgmt->readIO = 0;
  mgmt->writeIO = 0;
  mgmt->dirtyFlags = (bool *)calloc(numPages, sizeof(bool));
  mgmt->fixCounts = (int *)calloc(numPages, sizeof(int));

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

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  for (int i = 0; i < bm->numPages; i++) {
    if (mgmt->frames[i].pageNum == page->pageNum) {
      if (mgmt->fixCounts[i] == 0)
        return RC_PAGE_NOT_PINNED;

      mgmt->fixCounts[i]--;
      return RC_OK;
    }
  }

  return RC_PAGE_NOT_FOUND;
}

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

int findFrameToReplace(BM_BufferPool *const bm)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  if (bm->strategy == RS_FIFO) {
    int frameNum = mgmt->fifoQueue[0];

    for (int i = 0; i < bm->numPages - 1; i++)
      mgmt->fifoQueue[i] = mgmt->fifoQueue[i + 1];

    mgmt->fifoQueue[bm->numPages - 1] = frameNum;

    return frameNum;
  }
  else if (bm->strategy == RS_LRU) {
    int minTimestamp = mgmt->timestamps[0], minIndex = 0;

    for (int i = 1; i < bm->numPages; i++) {
      if (mgmt->timestamps[i] < minTimestamp) {
        minTimestamp = mgmt->timestamps[i];
        minIndex = i;
      }
    }

    mgmt->timestamps[minIndex] = ++mgmt->currentTimestamp;

    return minIndex;
  }

  return NO_PAGE;
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
	   const PageNumber pageNum)
{
  BM_MgmtData *mgmt = (BM_MgmtData *)bm->mgmtData;

  for (int i = 0; i < bm->numPages; i++) {
    if (mgmt->frames[i].pageNum == pageNum) {
      page->pageNum = pageNum;
      page->data = mgmt->frames[i].data;
      mgmt->fixCounts[i]++;
      return RC_OK;
    }
  }

  int frameNum = findFrameToReplace(bm);
  if (frameNum == NO_PAGE)
    return RC_BUFFER_POOL_FULL;

  if (mgmt->dirtyFlags[frameNum])
    forcePage(bm, &mgmt->frames[frameNum]);
  if(mgmt->frames[frameNum].data != NULL) {
    free(mgmt->frames[frameNum].data);
  }
  mgmt->frames[frameNum].data = malloc(PAGE_SIZE);
  readBlock(pageNum, &(mgmt->fileHandle), mgmt->frames[frameNum].data);
  mgmt->frames[frameNum].pageNum = pageNum;
  mgmt->fixCounts[frameNum] = 1;
  mgmt->readIO++;

  page->pageNum = pageNum;
  page->data = mgmt->frames[frameNum].data;

  return RC_OK;
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
