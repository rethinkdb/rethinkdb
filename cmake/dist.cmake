# Copyright 2026 RethinkDB, all rights reserved.
#
# Build a distribution package for the server.
#
# This file runs under `cmake -P`, which does not inherit policies from
# CMakeLists.txt. Without this, CMP0057 stays unset and `if(IN_LIST)` is a
# syntax error.

cmake_minimum_required(VERSION 3.24)

if(NOT SOURCE_DIR OR NOT BINARY_DIR OR NOT RETHINKDB_VERSION OR NOT PROTO_CC OR NOT PROTO_H)
    message(FATAL_ERROR "dist.cmake is missing a required -D argument")
endif()

set(stage "${BINARY_DIR}/packages/rethinkdb-${RETHINKDB_VERSION}")
set(archive "${BINARY_DIR}/packages/rethinkdb-${RETHINKDB_VERSION}.tgz")
file(REMOVE_RECURSE "${stage}")
file(MAKE_DIRECTORY "${stage}" "${BINARY_DIR}/packages")

foreach(item IN ITEMS
        demos packaging scripts src test cmake
        CMakeLists.txt CMakePresets.json COPYRIGHT NOTES.md README.md)
    if(EXISTS "${SOURCE_DIR}/${item}")
        file(COPY "${SOURCE_DIR}/${item}" DESTINATION "${stage}")
    else()
        message(FATAL_ERROR "Source distribution is missing ${item}")
    endif()
endforeach()

file(WRITE "${stage}/VERSION.OVERRIDE" "${RETHINKDB_CODE_VERSION}")
file(WRITE "${stage}/cmake-initial-cache.cmake"
"set(RETHINKDB_FETCH_RE2 ON CACHE BOOL \"\" FORCE)
set(RETHINKDB_FETCH_GTEST ON CACHE BOOL \"\" FORCE)
set(RETHINKDB_FETCH_BOOST ON CACHE BOOL \"\" FORCE)
set(RETHINKDB_FETCH_QUICKJS ON CACHE BOOL \"\" FORCE)
set(RETHINKDB_FETCH_JEMALLOC ON CACHE BOOL \"\" FORCE)
set(CMAKE_INSTALL_PREFIX \"/usr\" CACHE PATH \"\" FORCE)
set(CMAKE_INSTALL_SYSCONFDIR \"/etc\" CACHE PATH \"\" FORCE)
set(RETHINKDB_LOCALSTATEDIR \"/var\" CACHE PATH \"\" FORCE)
")

set(copied "")
if(EXISTS "${BINARY_DIR}/dist-support.txt")
    file(STRINGS "${BINARY_DIR}/dist-support.txt" support_lines)
    foreach(line IN LISTS support_lines)
        if(line STREQUAL "")
            continue()
        endif()
        string(REPLACE "\t" ";" fields "${line}")
        list(GET fields 0 name)
        list(GET fields 1 path)
        if(NOT IS_DIRECTORY "${path}")
            message(FATAL_ERROR "Fetched source for ${name} is missing: ${path}")
        endif()
        file(COPY "${path}/" DESTINATION "${stage}/external/${name}")
        list(APPEND copied "${name}")
    endforeach()
endif()

set(required re2 gtest boost quickjs jemalloc)
if(BUILD_PORTABLE)
    list(APPEND required protobuf)
endif()
foreach(name IN LISTS required)
    if(NOT name IN_LIST copied)
        string(TOUPPER "${name}" name_upper)
        message(FATAL_ERROR
            "Source package is missing fetched ${name}. "
            "Reconfigure with -DRETHINKDB_FETCH_${name_upper}=ON and build the dist target again.")
    endif()
endforeach()

file(MAKE_DIRECTORY "${stage}/precompiled/proto/rdb_protocol")
if(NOT EXISTS "${PROTO_CC}" OR NOT EXISTS "${PROTO_H}")
    message(FATAL_ERROR "Generated protobuf sources are missing. Build ql2_proto first.")
endif()
file(COPY "${PROTO_CC}" "${PROTO_H}" DESTINATION "${stage}/precompiled/proto/rdb_protocol")

file(REMOVE "${archive}")
execute_process(
    COMMAND tar -czf "${archive}" -C "${BINARY_DIR}/packages" "rethinkdb-${RETHINKDB_VERSION}"
    RESULT_VARIABLE tar_result
)
if(NOT tar_result EQUAL 0)
    message(FATAL_ERROR "tar failed while creating ${archive}")
endif()
message(STATUS "Wrote ${archive}")
