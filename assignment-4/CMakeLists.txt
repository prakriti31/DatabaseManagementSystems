cmake_minimum_required(VERSION 3.29)
project(assignment_4 C)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

add_executable(assignment_4
        btree_mgr.c
        btree_mgr.h
        buffer_mgr.c
        buffer_mgr.h
        buffer_mgr_stat.c
        buffer_mgr_stat.h
        dberror.c
        dberror.h
        dt.h
        expr.c
        expr.h
        record_mgr.c
        record_mgr.h
        rm_serializer.c
        storage_mgr.c
        storage_mgr.h
        tables.h
        test_assign4_1.c
        test_helper.h
)