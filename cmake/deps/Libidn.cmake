# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build libidn.

include(ExternalProject)

set(_rethinkdb_idn_version "1.38")
set(_rethinkdb_idn_install "${CMAKE_BINARY_DIR}/external/libidn")
set(_rethinkdb_idn_configure
    "<SOURCE_DIR>/configure"
    "--prefix=${_rethinkdb_idn_install}"
    "--enable-static"
    "--disable-shared"
)

if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_TARGET)
    list(APPEND _rethinkdb_idn_configure "--host=${CMAKE_CXX_COMPILER_TARGET}")
endif()

ExternalProject_Add(libidn_ext
    URL "https://ftp.gnu.org/gnu/libidn/libidn-${_rethinkdb_idn_version}.tar.gz"
    URL_HASH "SHA256=de00b840f757cd3bb14dd9a20d5936473235ddcba06d4bc2da804654b8bbf0f6"
    DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
    INSTALL_DIR "${_rethinkdb_idn_install}"
    ${RETHINKDB_EXTERNAL_LOG_ARGS}
    CONFIGURE_COMMAND ${_rethinkdb_idn_configure}
    BUILD_COMMAND "${RETHINKDB_GNU_MAKE}"
    INSTALL_COMMAND "${RETHINKDB_GNU_MAKE}" install
    BUILD_BYPRODUCTS "${_rethinkdb_idn_install}/lib/libidn.a"
)

add_library(RethinkDB_idn STATIC IMPORTED GLOBAL)
add_library(RethinkDB::idn ALIAS RethinkDB_idn)
file(MAKE_DIRECTORY "${_rethinkdb_idn_install}/include")
set_target_properties(RethinkDB_idn PROPERTIES
    IMPORTED_LOCATION "${_rethinkdb_idn_install}/lib/libidn.a"
)

target_include_directories(RethinkDB_idn SYSTEM INTERFACE
    "${_rethinkdb_idn_install}/include")
if(APPLE)
    find_library(RETHINKDB_ICONV_LIBRARY iconv)
    if(RETHINKDB_ICONV_LIBRARY)
        set_property(TARGET RethinkDB_idn PROPERTY INTERFACE_LINK_LIBRARIES "${RETHINKDB_ICONV_LIBRARY}")
    endif()
endif()

rethinkdb_external_dependency(libidn_ext)
message(STATUS "libidn ${_rethinkdb_idn_version} will be built")
