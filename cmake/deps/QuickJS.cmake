# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build QuickJS.

include(ExternalProject)

set(_rethinkdb_quickjs_tag "95d6dcf6358a74e9cbe04eade7c383f43ba306cb")
set(_rethinkdb_quickjs_install "${CMAKE_BINARY_DIR}/external/quickjs")
set(_rethinkdb_quickjs_patches
    "${CMAKE_SOURCE_DIR}/cmake/deps/patches/quickjs_0-macos-min-version.patch"
    "${CMAKE_SOURCE_DIR}/cmake/deps/patches/quickjs_1-no-atomics.patch"
    "${CMAKE_SOURCE_DIR}/cmake/deps/patches/quickjs_2-no-workers.patch"
)

set(_rethinkdb_quickjs_apple 0)
if(APPLE)
    set(_rethinkdb_quickjs_apple 1)
endif()

if(WIN32)
    list(APPEND _rethinkdb_quickjs_patches
        "${CMAKE_SOURCE_DIR}/cmake/deps/patches/quickjs_3-msvc-mt.patch")
    if(CMAKE_VS_MSBUILD_COMMAND)
        set(RETHINKDB_MSBUILD "${CMAKE_VS_MSBUILD_COMMAND}")
    else()
        find_program(RETHINKDB_MSBUILD NAMES MSBuild msbuild REQUIRED)
    endif()
    find_program(RETHINKDB_PREMAKE NAMES premake5 REQUIRED)
    set(_rethinkdb_quickjs_byproduct "${_rethinkdb_quickjs_install}/lib/quickjs.lib")
else()
    set(RETHINKDB_MSBUILD "")
    set(RETHINKDB_PREMAKE "")
    set(_rethinkdb_quickjs_byproduct "${_rethinkdb_quickjs_install}/lib/quickjs/libquickjs.a")
endif()

ExternalProject_Add(quickjs_ext
    URL "https://github.com/rethinkdb/quickjspp/archive/${_rethinkdb_quickjs_tag}.tar.gz"
    URL_HASH "SHA256=ce498cdd2879ac2ad1e3755da8642867f9363e27d41e9aea4aad6692fde8f138"
    DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
    INSTALL_DIR "${_rethinkdb_quickjs_install}"
    ${RETHINKDB_EXTERNAL_LOG_ARGS}
    PATCH_COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=<SOURCE_DIR>"
        "-DPATCHES=${_rethinkdb_quickjs_patches}"
        -P "${CMAKE_SOURCE_DIR}/cmake/apply_patches.cmake"
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=<SOURCE_DIR>"
        "-DINSTALL_DIR=<INSTALL_DIR>"
        "-DMAKE=${RETHINKDB_GNU_MAKE}"
        "-DCC=${CMAKE_C_COMPILER}"
        "-DAR=${CMAKE_AR}"
        "-DCONFIG=$<CONFIG>"
        "-DPLATFORM=${RETHINKDB_WINDOWS_PLATFORM}"
        "-DTOOLSET=${RETHINKDB_WINDOWS_TOOLSET}"
        "-DSDK=${RETHINKDB_WINDOWS_SDK}"
        "-DPREMAKE=${RETHINKDB_PREMAKE}"
        "-DMSBUILD=${RETHINKDB_MSBUILD}"
        "-DRETHINKDB_APPLE=${_rethinkdb_quickjs_apple}"
        -P "${CMAKE_SOURCE_DIR}/cmake/deps/build_quickjs.cmake"
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${_rethinkdb_quickjs_byproduct}"
)

file(MAKE_DIRECTORY "${_rethinkdb_quickjs_install}/include")
add_library(RethinkDB_quickjs STATIC IMPORTED GLOBAL)
add_library(RethinkDB::quickjs ALIAS RethinkDB_quickjs)
set_target_properties(RethinkDB_quickjs PROPERTIES
    IMPORTED_LOCATION "${_rethinkdb_quickjs_byproduct}"
)
target_include_directories(RethinkDB_quickjs SYSTEM INTERFACE
    "${_rethinkdb_quickjs_install}/include"
)
if(NOT WIN32)
    set_property(TARGET RethinkDB_quickjs PROPERTY INTERFACE_LINK_LIBRARIES
        "m;${CMAKE_DL_LIBS};Threads::Threads")
endif()

rethinkdb_external_dependency(quickjs_ext)
ExternalProject_Get_Property(quickjs_ext SOURCE_DIR)
rethinkdb_record_dist_source(quickjs "${SOURCE_DIR}")
message(STATUS "QuickJS ${_rethinkdb_quickjs_tag} will be built")
