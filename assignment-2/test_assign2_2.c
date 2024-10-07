#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"
#include "dberror.h"
#include "test_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// var to store the current test's name
char *testName;

// check whether two the content of a buffer pool is the same as an expected content
// (given in the format produced by sprintPoolContent)
#define ASSERT_EQUALS_POOL(expected,bm,message)                    \
  do {                                                             \
    char *real;                                                    \
    char *_exp = (char *) (expected);                              \
    real = sprintPoolContent(bm);                                  \
    if (strcmp((_exp),real) != 0)                                  \
    {                                                              \
      printf("[%s-%s-L%i-%s] FAILED: expected <%s> but was <%s>: %s\n",TEST_INFO, _exp, real, message); \
      free(real);                                                  \
      exit(1);                                                     \
    }                                                              \
    printf("[%s-%s-L%i-%s] OK: expected <%s> and was <%s>: %s\n",TEST_INFO, _exp, real, message); \
    free(real);                                                    \
  } while(0)

// test and helper methods
static void createDummyPages(BM_BufferPool *bm, int num);
static void testCLOCK(void);

// main method
int
main (void)
{
  initStorageManager();
  testName = "";

  testCLOCK();

  return 0;
}

// create n pages with content "Page X" where X is the page number
void
createDummyPages(BM_BufferPool *bm, int num)
{
  int i;
  BM_PageHandle *h = MAKE_PAGE_HANDLE();

  CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL));

  for (i = 0; i < num; i++)
  {
    CHECK(pinPage(bm, h, i));
    sprintf(h->data, "Page %d", h->pageNum);
    CHECK(markDirty(bm, h));
    CHECK(unpinPage(bm,h));
  }

  CHECK(shutdownBufferPool(bm));
  free(h);
}

// test the CLOCK page replacement strategy
void testCLOCK(void) {
    // expected results
    const char *poolContents[] = {
        "[0 1],[-1 0],[-1 0]",
        "[0 1],[1 1],[-1 0]",
        "[0 1],[1 1],[2 1]",
        "[3 1],[1 1],[2 1]",
        "[3 1],[4 1],[2 1]",
        "[3 0],[4 1],[5 1]",
        "[6 1],[4 1],[5 1]",
        "[6 1],[7 1],[5 1]",
        "[6 0],[7 1],[8 1]"
    };
    const int requests[] = {0,1,2,3,4,5,6,7,8};
    const int numLinearRequests = 9;

    int i;
    BM_BufferPool *bm = MAKE_POOL();
    BM_PageHandle *h = MAKE_PAGE_HANDLE();
    testName = "Testing CLOCK page replacement";

    CHECK(createPageFile("testbuffer.bin"));
    createDummyPages(bm, 100);
    CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_CLOCK, NULL));

    // Reading pages to bring them into buffer pool
    for(i = 0; i < numLinearRequests; i++) {
        pinPage(bm, h, requests[i]);
        if (i != 0)
            unpinPage(bm, h);
        ASSERT_EQUALS_POOL(poolContents[i], bm, "check pool content");
    }

    // Check number of read and write I/Os
    ASSERT_EQUALS_INT(9, getNumReadIO(bm), "check number of read I/Os");
    ASSERT_EQUALS_INT(0, getNumWriteIO(bm), "check number of write I/Os");

    // Pin some pages again to change their reference bit
    CHECK(pinPage(bm, h, 0));
    CHECK(pinPage(bm, h, 3));
    CHECK(pinPage(bm, h, 4));
    ASSERT_EQUALS_POOL("[0 1],[3 1],[4 1]", bm, "check pool content after re-pinning pages");

    // Unpin all pages
    CHECK(unpinPage(bm, h));
    CHECK(unpinPage(bm, h));
    CHECK(unpinPage(bm, h));

    // Request a new page to test CLOCK replacement
    CHECK(pinPage(bm, h, 9));
    ASSERT_EQUALS_POOL("[0 0],[3 1],[9 1]", bm, "check pool content after CLOCK replacement");

    // Clean up
    CHECK(shutdownBufferPool(bm));
    CHECK(destroyPageFile("testbuffer.bin"));
    free(bm);
    free(h);
    TEST_DONE();
}