#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"
// New test file here

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testSinglePageContent(void);

// Custom Test cases
static void testAppendEmptyPage(void);
static void testReadLastBlock(void);
static void testReadBeyondEOF(void);
static void testWriteReadBlockAtPosition(void);

/* main function running all tests */
int
main (void)
{
  testName = "";

  initStorageManager();
  //
  testCreateOpenClose();
  testSinglePageContent();
  // Custom test cases
  testAppendEmptyPage();
  testReadLastBlock();
  testReadBeyondEOF();
  testWriteReadBlockAtPosition();
  // printf("PAGE_SIZE: %d",PAGE_SIZE);

  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void
testCreateOpenClose(void)
{
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile (TESTPF));

  TEST_CHECK(openPageFile (TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");

  TEST_DONE();
}

/* Try to create, open, and close a page file */
void
testSinglePageContent(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");

  // read first page into handle
  TEST_CHECK(readFirstBlock (&fh, ph));
  // the page should be empty (zero bytes)
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");

  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  // destroy new page file
  TEST_CHECK(destroyPageFile (TESTPF));

  TEST_DONE();
}

void
testAppendEmptyPage(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;

  testName = "test append empty page";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // Create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));

  // Append an empty block to the file
  TEST_CHECK(appendEmptyBlock(&fh));
  ASSERT_TRUE((fh.totalNumPages == 2), "file should have 2 pages after appending one empty page");

  // Read the second page to ensure it's empty
  TEST_CHECK(readBlock(1, &fh, ph));
  for (int i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in newly appended page");

  // Cleanup
  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));

  free(ph);
  TEST_DONE();
}

/* Try to read the last block of a multi-page file */
void
testReadLastBlock(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test read last block";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // Create a new page file and write two pages of content
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));

  // Write to the first page
  for (i = 0; i < PAGE_SIZE; i++)
    ph[i] = 'A';
  TEST_CHECK(writeBlock(0, &fh, ph));

  // Append and write to the second page
  TEST_CHECK(appendEmptyBlock(&fh));
  for (i = 0; i < PAGE_SIZE; i++)
    ph[i] = 'B';
  TEST_CHECK(writeBlock(1, &fh, ph));

  // Read the last block and verify its content
  TEST_CHECK(readLastBlock(&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 'B'), "expected 'B' byte in the last page read from disk");

  // Cleanup
  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));

  free(ph);
  TEST_DONE();
}

void
testReadBeyondEOF(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;

  testName = "test read beyond EOF";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // Create a new page file and open it
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));

  // Try reading beyond the first (and only) page of the file
  ASSERT_TRUE((readBlock(2, &fh, ph) != RC_OK), "reading beyond the end of the file should return an error");

  // Cleanup
  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));

  free(ph);
  TEST_DONE();
}

/* Test writing and reading at a specific page position in a multi-page file */
void
testWriteReadBlockAtPosition(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test write and read block at specific position";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // Create a new page file and append two pages
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  TEST_CHECK(appendEmptyBlock(&fh));
  TEST_CHECK(appendEmptyBlock(&fh));

  // Write distinct data to the second page (index 2)
  for (i = 0; i < PAGE_SIZE; i++)
    ph[i] = 'C';
  TEST_CHECK(writeBlock(2, &fh, ph));

  // Now read back the second page and verify the content
  TEST_CHECK(readBlock(2, &fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 'C'), "expected 'C' byte in the second page");

  // Ensure that the first page is unaffected
  TEST_CHECK(readBlock(1, &fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "first page should still be empty");

  // Cleanup
  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));

  free(ph);
  TEST_DONE();
}