cmake_minimum_required(VERSION 3.29)
project(assignment_2 C)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address")

# Create the first executable for test_assign2_1
add_executable(test_2_1 storage_mgr.c
        buffer_mgr.c
        buffer_mgr_stat.c
        test_assign2_1.c
        dberror.c
        test_helper.h)
