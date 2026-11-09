# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build jemalloc.

include(ExternalProject)

set(_rethinkdb_jemalloc_version "5.2.1")
string(CONCAT _rethinkdb_jemalloc_url
    "https://github.com/jemalloc/jemalloc/releases/download/${_rethinkdb_jemalloc_version}"
    "/jemalloc-${_rethinkdb_jemalloc_version}.tar.bz2")
set(_rethinkdb_jemalloc_install "${CMAKE_BINARY_DIR}/external/jemalloc")
set(_rethinkdb_jemalloc_configure
    "<SOURCE_DIR>/configure"
    "--prefix=${_rethinkdb_jemalloc_install}"
    "--libdir=${_rethinkdb_jemalloc_install}/lib"
)

if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_TARGET)
    list(APPEND _rethinkdb_jemalloc_configure "--host=${CMAKE_CXX_COMPILER_TARGET}")
endif()

ExternalProject_Add(jemalloc_ext
    URL "${_rethinkdb_jemalloc_url}"
    URL_HASH "SHA256=34330e5ce276099e2e8950d9335db5a875689a4c6a56751ef3b1d8c537f887f6"
    DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
    INSTALL_DIR "${_rethinkdb_jemalloc_install}"
    ${RETHINKDB_EXTERNAL_LOG_ARGS}
    CONFIGURE_COMMAND ${_rethinkdb_jemalloc_configure}
    BUILD_COMMAND "${RETHINKDB_GNU_MAKE}"
    INSTALL_COMMAND "${RETHINKDB_GNU_MAKE}" install
    BUILD_BYPRODUCTS "${_rethinkdb_jemalloc_install}/lib/libjemalloc.a"
)

add_library(RethinkDB_jemalloc STATIC IMPORTED GLOBAL)
add_library(RethinkDB::jemalloc ALIAS RethinkDB_jemalloc)
set_target_properties(RethinkDB_jemalloc PROPERTIES
    IMPORTED_LOCATION "${_rethinkdb_jemalloc_install}/lib/libjemalloc.a"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_property(TARGET RethinkDB_jemalloc PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS}")
endif()

rethinkdb_external_dependency(jemalloc_ext)
ExternalProject_Get_Property(jemalloc_ext SOURCE_DIR)
rethinkdb_record_dist_source(jemalloc "${SOURCE_DIR}")
message(STATUS "jemalloc ${_rethinkdb_jemalloc_version} will be built")
