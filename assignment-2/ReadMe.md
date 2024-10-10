# Buffer Manager Implementation

This project implements a buffer manager for a database system. The buffer manager efficiently manages a buffer pool of pages in memory, handles page replacement strategies, and coordinates reads and writes to the underlying storage manager.

## Files
- `storage_mgr.c` and `storage_mgr.h`: Implement the storage manager functionality.
- `buffer_mgr.c` and `buffer_mgr.h`: Implement the buffer manager functionality.
- `buffer_mgr_stat.c` and `buffer_mgr_stat.h`: Provide statistics and debugging functions for the buffer manager.
- `dberror.c` and `dberror.h`: Handle error codes and messages.
- `test_assign2_1.c`: Contains test cases for the buffer manager implementation.
- `test_helper.h`: Provides helper macros for testing.
- `dt.h`: Defines data types used in the project.

## Running Tests
After compilation, run the tests using:

./test_assign2_1

## Functionality
The buffer manager implements the following key features:
1. Buffer pool initialization and shutdown
2. Page pinning and unpinning
3. Marking pages as dirty
4. Forcing single pages or entire pool to disk
5. FIFO and LRU page replacement strategies

## Contribution
The following functions in `buffer_mgr.c` were implemented by:

- Yash Vardhan Sharma:
    - `initBufferPool`
    - `shutdownBufferPool`
    - `forceFlushPool`

- Prakriti Sharma:
    - `updateLRUOrder`
    - `getFrameContents`

- Kamakshya Nanda:
    - `getDirtyFlags`
    - `getFixCounts`
    - `getNumReadIO`
    - `getNumWriteIO`

- All team member contribution(Yash, Prakriti and Kamakshya):
  - `pinPage`
  - `unpinPage`
  - `forcePage`
  - `findFrameToReplace`
  - `markDirty`

All team members collaborated on the overall design, testing, and debugging of the buffer manager implementation.

## Notes
- The implementation assumes that the page size is 4096 bytes (PAGE_SIZE).
- Error handling is done using the RC (Return Code) system defined in `dberror.h`.
- The buffer manager relies on the storage manager for actual disk I/O operations.