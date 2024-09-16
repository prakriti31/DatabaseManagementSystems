cmake_minimum_required(VERSION 3.29)
project(assign_1 C)

set(CMAKE_C_STANDARD 17)

add_executable(assign_1 storage_mgr.c
        dberror.c
        dberror.h
        storage_mgr.h
        test_helper.h
        test_assign1_1.c)
