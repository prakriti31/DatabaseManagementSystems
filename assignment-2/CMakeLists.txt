cmake_minimum_required(VERSION 3.29)
project(assignment_2 C)

set(CMAKE_C_STANDARD 11)

# Create the first executable for test_assign2_1
add_executable(test_2_1 storage_mgr.c
        buffer_mgr.c
        buffer_mgr_stat.c
        test_assign2_1.c
        dberror.c)

# Create the second executable for test_assign2_2
add_executable(test_2_2 storage_mgr.c
        buffer_mgr.c
        buffer_mgr_stat.c
        test_assign2_2.c
        dberror.c)