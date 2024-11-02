cmake_minimum_required(VERSION 3.29)
project(assign3_record_manager C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(assign3_record_manager
        buffer_mgr.c
        buffer_mgr.h
        buffer_mgr_stat.c
        buffer_mgr_stat.h
        dberror.c
        dberror.h
        dt.h
        expr.c
        expr.h
        record_mgr.h
        rm_serializer.c
        storage_mgr.c
        storage_mgr.h
        tables.h
        test_assign3_1.c
        #    test_expr.c
        test_helper.h
        record_mgr.c
        storage_mgr.c
        buffer_mgr.c
)
