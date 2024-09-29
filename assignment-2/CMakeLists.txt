cmake_minimum_required(VERSION 3.29)
project(assignment_2 C)

set(CMAKE_C_STANDARD 11)

add_executable(assignment_2 storage_mgr.c
        buffer_mgr.c
#        buffer_mgr_stat.c
        test_assign2_1.c
        dberror.c)
