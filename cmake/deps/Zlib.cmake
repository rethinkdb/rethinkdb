# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build zlib.

include(ExternalProject)

set(_rethinkdb_zlib_version "1.2.11")
set(_rethinkdb_zlib_install "${CMAKE_BINARY_DIR}/external/zlib")

if(WIN32)
    set(_rethinkdb_zlib_lib "${_rethinkdb_zlib_install}/lib/zlib.lib")

    ExternalProject_Add(zlib_ext
        URL "https://zlib.net/fossils/zlib-${_rethinkdb_zlib_version}.tar.gz"
        URL_HASH "SHA256=c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1"
        DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
        INSTALL_DIR "${_rethinkdb_zlib_install}"
        ${RETHINKDB_EXTERNAL_LOG_ARGS}
        PATCH_COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_DIR=<SOURCE_DIR>"
            "-DPATCHES=${CMAKE_SOURCE_DIR}/cmake/deps/patches/zlib_0-makefile-runtime-option.patch"
            -P "${CMAKE_SOURCE_DIR}/cmake/apply_patches.cmake"
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_DIR=<SOURCE_DIR>"
            "-DCONFIG=$<CONFIG>"
            -P "${CMAKE_SOURCE_DIR}/cmake/deps/build_zlib_windows.cmake"
        INSTALL_COMMAND
            "${CMAKE_COMMAND}" -E make_directory "<INSTALL_DIR>/lib"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "<INSTALL_DIR>/include"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "<SOURCE_DIR>/zlib.lib" "<INSTALL_DIR>/lib/zlib.lib"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "<SOURCE_DIR>/zlib.h" "<INSTALL_DIR>/include/zlib.h"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "<SOURCE_DIR>/zconf.h" "<INSTALL_DIR>/include/zconf.h"
        BUILD_BYPRODUCTS "${_rethinkdb_zlib_lib}"
    )
else()
    set(_rethinkdb_zlib_lib "${_rethinkdb_zlib_install}/lib/libz.a")
    ExternalProject_Add(zlib_ext
        URL "https://zlib.net/fossils/zlib-${_rethinkdb_zlib_version}.tar.gz"
        URL_HASH "SHA256=c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1"
        DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
        INSTALL_DIR "${_rethinkdb_zlib_install}"
        ${RETHINKDB_EXTERNAL_LOG_ARGS}
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "<SOURCE_DIR>/configure" "--prefix=${_rethinkdb_zlib_install}" --static
        BUILD_COMMAND "${RETHINKDB_GNU_MAKE}"
        INSTALL_COMMAND "${RETHINKDB_GNU_MAKE}" install
        BUILD_BYPRODUCTS "${_rethinkdb_zlib_lib}"
    )
endif()

file(MAKE_DIRECTORY "${_rethinkdb_zlib_install}/include")
add_library(RethinkDB_zlib STATIC IMPORTED GLOBAL)
add_library(RethinkDB::zlib ALIAS RethinkDB_zlib)
set_target_properties(RethinkDB_zlib PROPERTIES
    IMPORTED_LOCATION "${_rethinkdb_zlib_lib}"
)
target_include_directories(RethinkDB_zlib SYSTEM INTERFACE
    "${_rethinkdb_zlib_install}/include")

rethinkdb_external_dependency(zlib_ext)
message(STATUS "zlib ${_rethinkdb_zlib_version} will be built")
