# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build Google Test.

include(FetchContent)

if(NOT BUILD_TESTING)
    return()
endif()

if(RETHINKDB_FETCH_GTEST)
    set(_rethinkdb_gtest_version "1.8.1")

    rethinkdb_fetch_source(googletest
        URL "https://github.com/google/googletest/archive/release-${_rethinkdb_gtest_version}.tar.gz"
        URL_HASH "SHA256=9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c"
    )

    set(RETHINKDB_GTEST_INCLUDE "${googletest_SOURCE_DIR}/googletest/include")
    add_library(RethinkDB_gtest STATIC
        "${googletest_SOURCE_DIR}/googletest/src/gtest-all.cc")
    add_library(RethinkDB::gtest ALIAS RethinkDB_gtest)

    target_include_directories(RethinkDB_gtest
        SYSTEM PUBLIC "${RETHINKDB_GTEST_INCLUDE}"
        PRIVATE "${googletest_SOURCE_DIR}/googletest"
    )

    set_target_properties(RethinkDB_gtest PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_compile_options(RethinkDB_gtest PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
        $<$<CXX_COMPILER_ID:MSVC>:/w>
    )

    if(NOT WIN32)
        target_link_libraries(RethinkDB_gtest PUBLIC Threads::Threads)
    endif()

    rethinkdb_record_dist_source(gtest "${googletest_SOURCE_DIR}")
    message(STATUS "Using Google Test ${_rethinkdb_gtest_version} from ${googletest_SOURCE_DIR}")
else()
    find_package(GTest REQUIRED)
    add_library(RethinkDB::gtest ALIAS GTest::gtest)
    if(TARGET GTest::gtest)
        get_target_property(RETHINKDB_GTEST_INCLUDE GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
    endif()
    message(STATUS "Using system Google Test")
endif()
